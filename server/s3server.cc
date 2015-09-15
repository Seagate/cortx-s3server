// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <stdint.h>
// #include <errno.h>
// // #include <evhtp.h>

// /* KD */
// #include <sys/stat.h>
// #include <math.h>

#include "clovis_helpers.h"
#include <openssl/md5.h>
#include "murmur3_hash.h"

/* protobuf */
#include "mero_object_header.pb.h"

#include "s3_router.h"

#define WEBSTORE "/home/seagate/webstore"

/* Program options */
#include <unistd.h>
const char   * bind_addr      = "0.0.0.0";
const char *clovis_local_addr = "localhost@tcp:12345:33:100";
const char *clovis_confd_addr = "localhost@tcp:12345:33:100";
const char *clovis_prof = "<0x7000000000000001:0>";
uint16_t bind_port      = 8081;

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

    printf("URI = %s\n", url);
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
  S3Router *router = (S3Router*)a;
  router->dispatch(req);
}


// static evhtp_res
// set_my_connection_handlers(evhtp_connection_t * conn, void * arg) {
//     // evhtp_set_hook(&conn->hooks, evhtp_hook_on_read, print_data, "derp");
//     return EVHTP_RES_OK;
// }

const char * optstr = "a:p:l:c:h";

const char * help   =
    "Options: \n"
    "  -h       : This help text\n"
    "  -a <str> : Bind Address             (default: 0.0.0.0)\n"
    "  -p <int> : Bind Port                (default: 8081)\n"
    "  -l <str> : clovis local address     (default: 10.0.2.15@tcp:12345:33:100)\n"
    "  -c <str> : clovis confd address     (default: 10.0.2.15@tcp:12345:33:100)\n";

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
            default:
                printf("Unknown opt %s\n", optarg);
                return -1;
        } /* switch */
    }
    printf("bind_addr = %s\n", bind_addr);
    printf("bind_port = %d\n", bind_port);
    printf("clovis_local_addr = %s\n", clovis_local_addr);
    printf("clovis_confd_addr = %s\n", clovis_confd_addr);

    return 0;
} /* parse_args */

int
main(int argc, char ** argv) {
    int rc = 0;

    if (parse_args(argc, argv) < 0) {
        exit(1);
    }

    /* Google protobuf */
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    /* protobuf example
    s3server::MeroObjectHeader obj_header, deserialized_obj;
    std::string streamed_data;

    obj_header.set_object_size(8192);
    obj_header.set_version("1.2");
    obj_header.SerializeToString(&streamed_data);

    deserialized_obj.ParseFromString(streamed_data);
    printf("\ndeserialized_obj.object_size = %ld\n", deserialized_obj.object_size());
    printf("deserialized_obj.version = %s\n", deserialized_obj.version().c_str());

    return 0;*/

    evbase_t * evbase = event_base_new();
    evhtp_t  * htp    = evhtp_new(evbase, NULL);

    S3Router *router = new S3Router();

    // So we can support queries like s3.com/bucketname?location or ?acl
    evhtp_set_parser_flags(htp, EVHTP_PARSE_QUERY_FLAG_ALLOW_NULL_VALS);

    evhtp_set_gencb(htp, s3_handler, NULL);
    // evhtp_set_gencb(htp, s3_handler, (void*)"someargnotused");

    /* Initilise mero and Clovis */
    rc = init_clovis(clovis_local_addr, clovis_confd_addr, clovis_prof);
    if (rc < 0) {
        printf("clovis_init failed!\n");
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
    delete router;

    return 0;
}
