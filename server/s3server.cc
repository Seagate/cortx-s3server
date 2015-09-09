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

/* HTTP HEAD */
int stat_from_my_store(evhtp_request_t *req)
{
    char in_store_path[1024];
    char c_file_size[64];
    struct stat st;

    sprintf(in_store_path, WEBSTORE"%s", req->uri->path->full);

    /* Get file size */
    if (stat(in_store_path, &st) == 0)
    {
        sprintf(c_file_size, "%ld", st.st_size);

        evhtp_headers_add_header(req->headers_out,
               evhtp_header_new("Content-Length", c_file_size, 0, 0));

        evhtp_send_reply(req, EVHTP_RES_OK);
    }
    else
    {
        evhtp_send_reply(req, EVHTP_RES_400);
    }
    return 0;
}

/* HTTP GET CLOVIS */
int stream_from_clovis(evhtp_request_t *req)
{
    char c_content_size[64];
    size_t total_bytes_returned = 0;
    int i = 0;
    struct m0_bufvec data_vec;
    void *data = NULL;
    size_t data_read = 0;
    int rc = 0;
    /* md5 for clients */
    MD5_CTX md5_ctx;
    unsigned char md5_digest[MD5_DIGEST_LENGTH + 1] = {'\0'};
    char md5_digest_chars[(MD5_DIGEST_LENGTH * 2) + 1] = {'\0'};

    /* Read data from clovis */
    int clovis_block_count = 2;
    int start_block_idx = 0;
    int clovis_block_size = 4096;
    struct m0_uint128 object_id;

    /* Use MurMur Hash to generate the OID */
    get_oid_using_hash(req->uri->path->full, &object_id);

    /* First read the metadata block */
    rc = clovis_read(object_id, clovis_block_size, 0, 1, &data_vec);
    if (rc != 0)
    {
        evbuffer_add_printf(req->buffer_out, "Object not found. [rc = %d]\n", rc);
        evhtp_send_reply(req, EVHTP_RES_400);
        return 0;
    }
    /* Interpret the size and deserialize the metadata header */
    uint32_t nbo_stream_len = 0;
    memcpy(&nbo_stream_len, data_vec.ov_buf[0], sizeof(uint32_t));
    s3server::MeroObjectHeader obj_header;
    std::string streamed_metadata(((const char*)data_vec.ov_buf[0] + sizeof(uint32_t)), ntohl(nbo_stream_len));

    obj_header.ParseFromString(streamed_metadata);

    m0_bufvec_free(&data_vec);
    /* Count Data blocks from data size */
    clovis_block_count = (obj_header.object_size() + (clovis_block_size - 1)) / clovis_block_size;

    /* Now read the data block */
    start_block_idx = 1; /* skip the metadata block */
    rc = clovis_read(object_id, clovis_block_size, start_block_idx, clovis_block_count, &data_vec);
    if (rc != 0)
    {
        evbuffer_add_printf(req->buffer_out, "Object not found.\n");
        evhtp_send_reply(req, EVHTP_RES_400);
        return 0;
    }

    struct evbuffer * b = evbuffer_new();

    /* consume the output */
    MD5_Init(&md5_ctx);
    for (i = 0; i < clovis_block_count; i++) {
        data_read = data_vec.ov_vec.v_count[i];
        if (i == (clovis_block_count - 1))
        {
            // For last chunk the actual valid data could be smaller.
            data_read = obj_header.object_size() % clovis_block_size;
            if (data_read == 0)
            {
                /* mean last chunk size was multiple of block size */
                data_read = clovis_block_size;
            }
        }
        MD5_Update(&md5_ctx, data_vec.ov_buf[i], data_read);
        total_bytes_returned = total_bytes_returned + data_read;
    }
    MD5_Final(md5_digest, &md5_ctx);
    buf_to_hex(md5_digest , MD5_DIGEST_LENGTH, md5_digest_chars, MD5_DIGEST_LENGTH * 2);

    sprintf(c_content_size, "%ld", obj_header.object_size() /*total_bytes_returned*/);

    evhtp_headers_add_header(req->headers_out,
           evhtp_header_new("Content-Length", c_content_size, 0, 0));
    evhtp_headers_add_header(req->headers_out,
           evhtp_header_new("etag", md5_digest_chars, 0, 0));

    evhtp_send_reply_start(req, EVHTP_RES_OK);
    for ( i = 0; i < clovis_block_count; i++)
    {
        data =  data_vec.ov_buf[i];
        data_read = data_vec.ov_vec.v_count[i];
        data_read = obj_header.object_size() % clovis_block_size;
        if (data_read == 0)
        {
            /* mean last chunk size was multiple of block size */
            data_read = clovis_block_size;
        }

        // evbuffer_add_printf(req->buffer_out, "clovis read bytes = %d\n", data_read);
        evbuffer_add(b, data, data_read);
        evhtp_send_reply_body(req, b);
    }
    // evhtp_send_reply(req, EVHTP_RES_OK);

    evhtp_send_reply_end(req);

    evbuffer_free(b);

    m0_bufvec_free(&data_vec);

    return 0;
}

/* HTTP GET */
int stream_from_my_store(evhtp_request_t *req)
{
    FILE * pFile = NULL;
    char in_store_path[1024];
    char c_file_size[64];
    int num_of_blocks = 0, i = 0;
    char data[4096];
    size_t data_read = 0;

    sprintf(in_store_path, WEBSTORE"%s", req->uri->path->full);

    pFile = fopen(in_store_path, "rb");
    if (pFile)
    {
        // evbuffer_add_printf(req->buffer_out, "File found [%s]\n", in_store_path);

        struct evbuffer * b = evbuffer_new();

        /* Get file size */
        struct stat st;
        stat(in_store_path, &st);
        sprintf(c_file_size, "%ld", st.st_size);

        evhtp_headers_add_header(req->headers_out,
               evhtp_header_new("Content-Length", c_file_size, 0, 0));

        num_of_blocks = (st.st_size + 4025) / 4026;
        // evbuffer_add_printf(req->buffer_out, "file size = %ld\n", st.st_size);
        // evbuffer_add_printf(req->buffer_out, "num_of_blocks = %ld\n", num_of_blocks);

        evhtp_send_reply_start(req, EVHTP_RES_OK);
        for ( i = 0; i < num_of_blocks; i++)
        {
            data_read = fread(data, 1, 4096, pFile);
            // evbuffer_add_printf(req->buffer_out, "fread bytes = %d\n", data_read);
            evbuffer_add(b, data, data_read);
            evhtp_send_reply_body(req, b);
            if (feof(pFile)) {
                // evbuffer_add_printf(req->buffer_out, "EOF File reached.\n");
                break;
            }
        }

        evhtp_send_reply_end(req);

        evbuffer_free(b);

        fclose(pFile);
    } else {
        evhtp_send_reply(req, EVHTP_RES_400);
    }
    return 0;
}

/* HTTP PUT */
int stream_to_clovis(evhtp_request_t *req)
{
    int rc = 0, content_length = 0;
    struct evbuffer_iovec *vec_in = NULL;

    int start_block_idx = 0;
    int clovis_block_size = 4096;
    int clovis_block_count = 0;
    int num_of_extents = 0; /* = clovis block count */
    struct m0_uint128 object_id;
    char *err_msg;
    /* md5 for clients */
    MD5_CTX md5_ctx;
    unsigned char md5_digest[MD5_DIGEST_LENGTH + 1] = {'\0'};
    char md5_digest_chars[(MD5_DIGEST_LENGTH * 2) + 1] = {'\0'};

    MD5_Init(&md5_ctx);

    /* Use MurMur Hash to generate the OID */
    get_oid_using_hash(req->uri->path->full, &object_id);

    content_length = evbuffer_get_length(req->buffer_in);
    /* xxx: replace with const char * evhtp_header_find(evhtp_headers_t * headers, const char * key) */
    num_of_extents = evbuffer_peek(req->buffer_in, content_length/*-1*/, NULL, NULL, 0);

    /* do the actual peek */
    vec_in = (struct evbuffer_iovec *)malloc(num_of_extents * sizeof(struct evbuffer_iovec));
    evbuffer_peek(req->buffer_in, content_length, NULL/*start of buffer*/, vec_in, num_of_extents);
    /* xxx - do we need to drain after consuming data?? */

    /* Create the metadata for the object to be streamed */
    s3server::MeroObjectHeader obj_header;
    std::string streamed_metadata;

    obj_header.set_object_size(content_length);
    obj_header.set_version("1.0"); /* default for now */
    obj_header.SerializeToString(&streamed_metadata);

    clovis_block_count = (obj_header.object_size() + (clovis_block_size - 1)) / clovis_block_size;
    rc = clovis_write(object_id, clovis_block_size, clovis_block_count, start_block_idx, num_of_extents, vec_in, streamed_metadata.c_str(), streamed_metadata.length(), &err_msg);
    if (rc == 0)
    {
        /* Compute MD5 of what we wrote */
        for (int i = 0; i < num_of_extents; i++) {
            MD5_Update(&md5_ctx, vec_in[i].iov_base, vec_in[i].iov_len);
        }
        MD5_Final(md5_digest, &md5_ctx);
        buf_to_hex(md5_digest , MD5_DIGEST_LENGTH, md5_digest_chars, MD5_DIGEST_LENGTH * 2);

        evhtp_headers_add_header(req->headers_out,
               evhtp_header_new("etag", md5_digest_chars, 0, 0));

        evhtp_send_reply(req, EVHTP_RES_OK);
    } else
    {
        evhtp_send_reply(req, EVHTP_RES_BADREQ);
        evbuffer_add_printf(req->buffer_out, "Internal Error: %s\n", err_msg);
    }

    free(vec_in);
    return 0;
}

int stream_to_my_store(evhtp_request_t *req)
{
    int content_length = 0;
    size_t num_of_extents = 0;
    struct evbuffer_iovec *vec_out = NULL;
    char *data = NULL;
    size_t i = 0;
    size_t count = 0;
    FILE * pFile;
    char in_store_path[1024];

    sprintf(in_store_path, WEBSTORE"%s", req->uri->path->full);

    pFile = fopen(in_store_path, "wb");

    content_length = evbuffer_get_length(req->buffer_in);
    num_of_extents = evbuffer_peek(req->buffer_in, content_length/*-1*/, NULL, NULL, 0);

    evbuffer_add_printf(req->buffer_out, "path = %s\n", in_store_path);
    evbuffer_add_printf(req->buffer_out, "num_of_extents = %ld\n", num_of_extents);

    /* do the actual peek */
    vec_out = (struct evbuffer_iovec *)malloc(num_of_extents * sizeof(struct evbuffer_iovec));
    evbuffer_peek(req->buffer_in, content_length, NULL/*start of buffer*/, vec_out, num_of_extents);

    for( i = 0; i < num_of_extents; i++) {
      data = (char*)vec_out[i].iov_base;
      count = vec_out[i].iov_len;
      fwrite(data, 1, count, pFile);
    }
    fclose(pFile);
    free(vec_out);
    evhtp_send_reply(req, EVHTP_RES_OK);
    return 0;
}

/* HTTP DELETE */
int rm_from_my_store(evhtp_request_t *req)
{
    char in_store_path[1024];
    struct stat st;
    struct m0_uint128 object_id;

    /* Use MurMur Hash to generate the OID */
    get_oid_using_hash(req->uri->path->full, &object_id);

    sprintf(in_store_path, WEBSTORE"%s", req->uri->path->full);

    /* Get file size */
    if (stat(in_store_path, &st) == 0)
    {
        remove(in_store_path);
        evhtp_send_reply(req, EVHTP_RES_OK);
    }
    else
    {
        clovis_delete(object_id);
        evhtp_send_reply(req, EVHTP_RES_OK);
    }
    return 0;
}

extern "C" void
s3_handler(evhtp_request_t * req, void * a) {
    Router::instance()->dispatch(req);
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

    return 0;
}
