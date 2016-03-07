/*
 * COPYRIGHT 2015 SEAGATE LLC
 *
 * THIS DRAWING/DOCUMENT, ITS SPECIFICATIONS, AND THE DATA CONTAINED
 * HEREIN, ARE THE EXCLUSIVE PROPERTY OF SEAGATE TECHNOLOGY
 * LIMITED, ISSUED IN STRICT CONFIDENCE AND SHALL NOT, WITHOUT
 * THE PRIOR WRITTEN PERMISSION OF SEAGATE TECHNOLOGY LIMITED,
 * BE REPRODUCED, COPIED, OR DISCLOSED TO A THIRD PARTY, OR
 * USED FOR ANY PURPOSE WHATSOEVER, OR STORED IN A RETRIEVAL SYSTEM
 * EXCEPT AS ALLOWED BY THE TERMS OF SEAGATE LICENSES AND AGREEMENTS.
 *
 * YOU SHOULD HAVE RECEIVED A COPY OF SEAGATE'S LICENSE ALONG WITH
 * THIS RELEASE. IF NOT PLEASE CONTACT A SEAGATE REPRESENTATIVE
 * http://www.seagate.com/contact
 *
 * Original author:  Kaustubh Deorukhkar   <kaustubh.deorukhkar@seagate.com>
 * Original author:  Rajesh Nambiar   <rajesh.nambiar@seagate.com>
 * Original creation date: 9-Nov-2015
 */

#include "clovis_helpers.h"
#include <openssl/md5.h>
#include "murmur3_hash.h"

#include <tuple>
#include <vector>

#include <event2/thread.h>

#include "evhtp_wrapper.h"
#include "s3_router.h"
#include "s3_request_object.h"
#include "s3_error_codes.h"
#include "s3_perf_logger.h"
#include "s3_timer.h"
#include "s3_log.h"

#define WEBSTORE "/home/seagate/webstore"

/* Program options */
#include <unistd.h>
const char   * bind_addr      = "0.0.0.0";
const char *clovis_local_addr = "localhost@tcp:12345:33:100";
const char *clovis_confd_addr = "localhost@tcp:12345:33:100";
const char *clovis_prof = "<0x7000000000000001:0>";
uint16_t bind_port      = 8081;
short    clovis_layout_id = 9;
char *s3_log_file;
char *s3_log_mode;
// S3 Auth service
const char *auth_ip_addr = "127.0.0.1";
uint16_t auth_port = 8085;

const char *log_level_str[S3_LOG_DEBUG] = {"FATAL", "ERROR", "WARN", "INFO", "DEBUG"};

FILE *fp_log;

//To be read from config file
int s3log_level = S3_LOG_INFO;

/* MD5 helper */
/* KD xxx - needed? intention? convert 16bytes to 32bytes readable string */
void buf_to_hex(unsigned char *md5_digest, int in_len, char *md5_digest_chars, int out_len)
{
  /* TODO FIXME */
  int i = 0;
  if (in_len * 2 != out_len)
  {
    memcpy(md5_digest_chars, "error computing md5",  out_len);
    return;
  }
  for (i = 0; i < in_len; i++)
  {
    sprintf((char *)(&md5_digest_chars[i*2]), "%02x", (int)md5_digest[i]);
  }
}

void get_oid_using_hash(const char* url, struct m0_uint128 *object_id)
{
    /* MurMur Hash */
    size_t len = 0;
    uint64_t hash128_64[2];

    s3_log(S3_LOG_INFO, "URI = %s\n", url);
    len = strlen(url);
    MurmurHash3_x64_128(url, len, 0, &hash128_64);

    /* Truncate higher 32 bits as they are used by mero/clovis */
    // hash128_64[0] = hash128_64[0] & 0x0000ffff;
    hash128_64[1] = hash128_64[1] & 0xffff0000;

    *object_id = M0_CLOVIS_ID_APP;
    object_id->u_hi = hash128_64[0];
    object_id->u_lo = object_id->u_lo & hash128_64[1];
    return;
}

// static int
// output_header(evhtp_header_t * header, void * arg) {
//     evbuf_t * buf = arg;

//     evbuffer_add_printf(buf, "print_kvs() key = '%s', val = '%s'\n",
//                         header->key, header->val);
//     return 0;
// }


// static evhtp_res
// print_data(evhtp_request_t * req, evbuf_t * buf, void * arg) {
//     static int i = 1;
//     evbuffer_add_printf(req->buffer_out, "Called i = %d\n", i++);
//     // evbuffer_add_buffer(req->buffer_out, buf);
//     // evbuffer_drain(buf, -1);
//     return EVHTP_RES_OK;
// }

extern "C" void
s3_handler(evhtp_request_t * req, void * a) {
  // placeholder, required to complete the request processing.
}

extern "C" evhtp_res
dispatch_request(evhtp_request_t * req, evhtp_headers_t * hdrs, void * arg ) {
    s3_log(S3_LOG_INFO, "RECEIVED Request headers.\n");

    S3Router *router = (S3Router*)arg;

    EvhtpInterface *evhtp_obj_ptr = new EvhtpWrapper();
    std::shared_ptr<S3RequestObject> s3_request = std::make_shared<S3RequestObject> (req, evhtp_obj_ptr);

    req->cbarg = s3_request.get();

    router->dispatch(s3_request);

    return EVHTP_RES_OK;
}

extern "C" evhtp_res
process_request_data(evhtp_request_t * req, evbuf_t * buf, void * arg) {
  s3_log(S3_LOG_DEBUG, "RECEIVED Request body for sock = %d\n", req->conn->sock);
  S3RequestObject* request = (S3RequestObject*)req->cbarg;

  if (request) {
    evbuf_t            * s3_buf = evbuffer_new();
    size_t bytes_received = evbuffer_get_length(buf);

    evbuffer_add_buffer(s3_buf, buf);

    request->notify_incoming_data(s3_buf);

    s3_log(S3_LOG_DEBUG, "got %zu bytes of data for sock = %d\n", bytes_received, req->conn->sock);
  } else {
    evhtp_unset_all_hooks(&req->conn->hooks);
    s3_log(S3_LOG_DEBUG, "S3 request failed, Ignoring data for this request \n");
  }

  return EVHTP_RES_OK;
}

extern "C" evhtp_res
set_s3_connection_handlers(evhtp_connection_t * conn, void * arg) {

    evhtp_set_hook(&conn->hooks, evhtp_hook_on_headers, (evhtp_hook)dispatch_request, arg);
    evhtp_set_hook(&conn->hooks, evhtp_hook_on_read, (evhtp_hook)process_request_data, arg);

    return EVHTP_RES_OK;
}

const char * optstr = "a:p:l:c:s:m:o:d:h:i";

const char * help   =
    "Options: \n"
    "  -h       : This help text\n"
    "  -a <str> : Bind Address             (default: 0.0.0.0)\n"
    "  -p <int> : Bind Port                (default: 8081)\n"
    "  -l <str> : clovis local address     (default: 10.0.2.15@tcp:12345:33:100)\n"
    "  -c <str> : clovis confd address     (default: 10.0.2.15@tcp:12345:33:100)\n"
    "  -s <str> : Auth Service address     (default: 127.0.0.1)\n"
    "  -d <int> : Auth Service port        (default: 8085)\n"
    "  -i <int> : Clovis layout id         (default: 9 (1MB))\n"
    "  -o <str> : S3 Log file              (default: stdout)\n"
    "  -m <str> : S3 Log Level             (DEBUG | INFO | ERROR | FATAL  default is : INFO)\n";

int
parse_args(int argc, char ** argv) {
    extern char * optarg;
    extern int    optind;
    extern int    opterr;
    extern int    optopt;
    int           c;

    while ((c = getopt(argc, argv, optstr)) != -1) {
        switch (c) {
            case 'h':
                printf("Usage: %s [opts]\n%s", argv[0], help);
                return -1;
            case 'a':
                bind_addr              = strdup(optarg);
                break;
            case 'p':
                bind_port              = atoi(optarg);
                break;
            case 'l':
                clovis_local_addr      = strdup(optarg);
                break;
            case 'c':
                clovis_confd_addr      = strdup(optarg);
                break;
            case 's':
                auth_ip_addr           = strdup(optarg);
                break;
            case 'd':
                auth_port             = atoi(optarg);
                break;
            case 'i':
                clovis_layout_id      = atoi(optarg);
                break;
            case 'o':
                s3_log_file           = strdup(optarg);
                break;
            case 'm':
                s3_log_mode            = strdup(optarg);
                break;
            default:
                printf("Unknown opt %s\n", optarg);
                return -1;
        } /* switch */
    }
    if(s3_log_file) {
      fp_log = std::fopen(s3_log_file, "a");
      if(fp_log == NULL) {
        printf("File opening of log %s failed\n",s3_log_file);
        return -1;
      }
    } else {
      fp_log = stdout;
    }

    s3_log(S3_LOG_INFO, "bind_addr = %s\n", bind_addr);
    s3_log(S3_LOG_INFO, "bind_port = %d\n", bind_port);
    s3_log(S3_LOG_INFO, "clovis_local_addr = %s\n", clovis_local_addr);
    s3_log(S3_LOG_INFO, "clovis_confd_addr = %s\n", clovis_confd_addr);
    s3_log(S3_LOG_INFO, "Auth server: %s\n",auth_ip_addr);
    s3_log(S3_LOG_INFO, "Auth server port: %d\n",auth_port);
    s3_log(S3_LOG_INFO, "clovis_layout_id = %d\n", clovis_layout_id);
    s3_log(S3_LOG_INFO, "s3_log_file = %s\n", s3_log_file);
    s3_log(S3_LOG_INFO, "s3_log_mode = %s\n", s3_log_mode);

    if(strcmp(s3_log_mode,"INFO") == 0) {
      s3log_level = S3_LOG_INFO;
    } else if(strcmp(s3_log_mode, "DEBUG") == 0) {
      s3log_level = S3_LOG_DEBUG;
    } else if(strcmp(s3_log_mode, "ERROR") == 0) {
      s3log_level = S3_LOG_ERROR;
    } else if(strcmp(s3_log_mode, "FATAL") == 0) {
      s3log_level = S3_LOG_FATAL;
    }
    return 0;
} /* parse_args */

int
main(int argc, char ** argv) {
    int rc = 0;

    if (parse_args(argc, argv) < 0) {
        exit(1);
    }

    // Load Any configs.
    S3ErrorMessages::init_messages();

    // Initilise loggers
#ifdef S3_ENABLE_PERF
    S3PerfLogger::initialize();
#endif

    evbase_t * evbase = event_base_new();
    evthread_use_pthreads();
    if (evthread_make_base_notifiable(evbase)<0) {
      s3_log(S3_LOG_ERROR, "Couldn't make base notifiable!");
      return 1;
    }
    evhtp_t  * htp    = evhtp_new(evbase, NULL);

    S3Router *router = new S3Router(new S3APIHandlerFactory(),
                                    new S3UriFactory());

    // So we can support queries like s3.com/bucketname?location or ?acl
    evhtp_set_parser_flags(htp, EVHTP_PARSE_QUERY_FLAG_ALLOW_NULL_VALS);

    // Main request processing (processing headers & body) is done in hooks
    evhtp_set_post_accept_cb(htp, set_s3_connection_handlers, router);

    // This handler is just like complete the request processing & respond
    evhtp_set_gencb(htp, s3_handler, router);

    /* Initilise mero and Clovis */
    rc = init_clovis(clovis_local_addr, clovis_confd_addr, clovis_prof, clovis_layout_id);
    if (rc < 0) {
        s3_log(S3_LOG_FATAL, "clovis_init failed!\n");
        return rc;
    }

    /* KD - setup for reading data */
    /* set a callback to set per-connection hooks (via a post_accept cb) */
    // evhtp_set_post_accept_cb(htp, set_my_connection_handlers, NULL);

#if 0
#ifndef EVHTP_DISABLE_EVTHR
    evhtp_use_threads(htp, NULL, 4, NULL);
#endif
#endif
    evhtp_bind_socket(htp, bind_addr, bind_port, 1024);

    event_base_loop(evbase, 0);

    evhtp_unbind_socket(htp);
    evhtp_free(htp);
    event_base_free(evbase);

    /* Clean-up */
    fini_clovis();
#ifdef S3_ENABLE_PERF
    S3PerfLogger::finalize();
#endif

    delete router;
    if(fp_log != NULL && fp_log != stdout) {
      std::fclose(fp_log);
    }
    return 0;
}
