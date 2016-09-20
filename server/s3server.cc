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

#include <openssl/md5.h>
#include "clovis_helpers.h"
#include "murmur3_hash.h"
#include "s3_uri_to_mero_oid.h"

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
#include "s3_option.h"
#include "s3_daemonize_server.h"
#include "fid/fid.h"

#define WEBSTORE "/home/seagate/webstore"

/* Program options */
#include <unistd.h>

evbase_t * global_evbase_handle;
extern struct m0_clovis_realm clovis_uber_realm;
struct m0_uint128 root_account_user_index_oid;

extern "C" void
s3_handler(evhtp_request_t * req, void * a) {
  // placeholder, required to complete the request processing.
  s3_log(S3_LOG_DEBUG, "Request Completed.\n");
}

extern "C" void on_client_conn_err_callback(evhtp_request_t * req, evhtp_error_flags errtype, void * arg) {
  s3_log(S3_LOG_DEBUG, "S3 Client disconnected.\n");
  S3RequestObject* request = (S3RequestObject*)req->cbarg;

  if (request) {
    request->client_has_disconnected();
  }
  evhtp_unset_all_hooks(&req->conn->hooks);
  evhtp_unset_all_hooks(&req->hooks);

  return;
}

extern "C" int
s3_log_header(evhtp_header_t * header, void * arg) {

    s3_log(S3_LOG_DEBUG, "http header(key = '%s', val = '%s')\n",
                        header->key, header->val);
    return 0;
}

extern "C" evhtp_res
dispatch_request(evhtp_request_t * req, evhtp_headers_t * hdrs, void * arg ) {
    s3_log(S3_LOG_INFO, "Received Request with uri [%s].\n", req->uri->path->full);

    if (req->uri->query_raw) {
      s3_log(S3_LOG_DEBUG, "Received Request with query params [%s].\n", req->uri->query_raw);
    }
    // Log http headers
    evhtp_headers_for_each(hdrs, s3_log_header, NULL);

    S3Router *router = (S3Router*)arg;

    EvhtpInterface *evhtp_obj_ptr = new EvhtpWrapper();
    std::shared_ptr<S3RequestObject> s3_request = std::make_shared<S3RequestObject> (req, evhtp_obj_ptr);

    if (S3Option::get_instance()->get_is_s3_shutting_down()) {
      // We are shutting down, so don't entertain new requests.
      s3_request->pause();
      evhtp_unset_all_hooks(&req->conn->hooks);
      // Send response with 'Service Unavailable' code.
      s3_log(S3_LOG_DEBUG, "sending 'Service Unavailable' response...\n");
      s3_request->set_out_header_value("Retry-After", "1");
      s3_request->send_response(S3HttpFailed503);
      return EVHTP_RES_OK;
    }

    req->cbarg = s3_request.get();

    evhtp_set_hook(&req->hooks, evhtp_hook_on_error, (evhtp_hook)on_client_conn_err_callback, NULL);

    router->dispatch(s3_request);

    return EVHTP_RES_OK;
}

extern "C" evhtp_res
process_request_data(evhtp_request_t * req, evbuf_t * buf, void * arg) {
  s3_log(S3_LOG_DEBUG, "Received Request body for sock = %d\n", req->conn->sock);

  S3RequestObject* request = (S3RequestObject*)req->cbarg;

  if (request) {
    evbuf_t            * s3_buf = evbuffer_new();
    size_t bytes_received = evbuffer_get_length(buf);
    s3_log(S3_LOG_DEBUG, "got %zu bytes of data for sock = %d\n", bytes_received, req->conn->sock);

    evbuffer_add_buffer(s3_buf, buf);

    request->notify_incoming_data(s3_buf);

  } else {
    evhtp_unset_all_hooks(&req->conn->hooks);
    evhtp_unset_all_hooks(&req->hooks);
    s3_log(S3_LOG_DEBUG, "S3 request failed, Ignoring data for this request \n");
  }

  return EVHTP_RES_OK;
}

extern "C" evhtp_res
set_s3_connection_handlers(evhtp_connection_t * conn, void * arg) {

    evhtp_set_hook(&conn->hooks, evhtp_hook_on_headers, (evhtp_hook)dispatch_request, arg);
    evhtp_set_hook(&conn->hooks, evhtp_hook_on_read, (evhtp_hook)process_request_data, NULL);

    return EVHTP_RES_OK;
}

void fatal_libevent(int err) {
  s3_log(S3_LOG_ERROR, "Fatal error occured in libevent, error = %d\n", err);
}

// This function will initialize global variable, should not be removed
void initialize_global_var() {
  struct m0_uint128 temp = {0ULL, 0ULL};
  temp.u_lo = 1;
  // reserving an oid for root index -- M0_CLOVIS_ID_APP + 1
  m0_uint128_add(&root_account_user_index_oid, &M0_CLOVIS_ID_APP, &temp);
  struct m0_fid index_fid = M0_FID_TINIT('i', root_account_user_index_oid.u_hi,
                                         root_account_user_index_oid.u_lo);
  root_account_user_index_oid.u_hi = index_fid.f_container;
  root_account_user_index_oid.u_lo = index_fid.f_key;
}

// This index will be holding the ids for the Account, User index
int create_s3_user_index(std::string index_name) {
  int rc;
  struct m0_clovis_op *ops[1] = {NULL};
  struct m0_clovis_idx idx;

  memset(&idx, 0, sizeof(idx));
  ops[0] = NULL;
  // reserving an oid for root index -- M0_CLOVIS_ID_APP + 1
  initialize_global_var();
  m0_clovis_idx_init(&idx, &clovis_uber_realm, &root_account_user_index_oid);
  m0_clovis_entity_create(&idx.in_entity, &ops[0]);
  m0_clovis_op_launch(ops, 1);

  rc = m0_clovis_op_wait(
      ops[0], M0_BITS(M0_CLOVIS_OS_FAILED, M0_CLOVIS_OS_STABLE), M0_TIME_NEVER);
  if (rc < 0) {
    if (rc != -EEXIST) {
      m0_clovis_op_fini(ops[0]);
      m0_clovis_op_free(ops[0]);
      return rc;
    }
  }
  m0_clovis_op_fini(ops[0]);
  m0_clovis_op_free(ops[0]);
  return 0;
}

int
main(int argc, char ** argv) {
  int rc = 0;
  const char  *bind_addr;
  uint16_t     bind_port;

  // Load Any configs.
  if (parse_and_load_config_options(argc, argv) < 0) {
    fprintf(stderr, "%s:%d:parse_and_load_config_options failed\n", __FILE__,
            __LINE__);
    exit(1);
  }

  // Init general purpose logger here but don't use it for non-FATAL logs
  // until we daemonize.
  rc = init_log(argv[0]);
  if (rc < 0) {
    fprintf(stderr, "%s:%d:init_log failed\n", __FILE__, __LINE__);
    exit(1);
  }

  S3ErrorMessages::init_messages();
  S3Option *option_instance = S3Option::get_instance();

  // Init perf logger
  if (option_instance->s3_performance_enabled()) {
    S3PerfLogger::initialize(option_instance->get_perf_log_filename());
  }

  S3Daemonize s3daemon;
  s3daemon.daemonize();
  s3daemon.register_signals();

  // dump the config
  option_instance->dump_options();

  // Call this function before creating event base
  evthread_use_pthreads();

  global_evbase_handle = event_base_new();
  option_instance->set_eventbase(global_evbase_handle);

  // Uncomment below api if we want to run libevent in debug mode
  // event_enable_debug_mode();

  if (evthread_make_base_notifiable(global_evbase_handle)<0) {
    s3_log(S3_LOG_ERROR, "Couldn't make base notifiable!");
    return 1;
  }
  evhtp_t  * htp    = evhtp_new(global_evbase_handle, NULL);
  event_set_fatal_callback(fatal_libevent);

  S3Router *router = new S3Router(new S3APIHandlerFactory(),
                                  new S3UriFactory());

  // So we can support queries like s3.com/bucketname?location or ?acl
  evhtp_set_parser_flags(htp, EVHTP_PARSE_QUERY_FLAG_ALLOW_NULL_VALS);

  // Main request processing (processing headers & body) is done in hooks
  evhtp_set_post_accept_cb(htp, set_s3_connection_handlers, router);

  // This handler is just like complete the request processing & respond
  evhtp_set_gencb(htp, s3_handler, router);

  bind_port = option_instance->get_s3_bind_port();
  bind_addr = option_instance->get_bind_addr().c_str();

  /* Initilise mero and Clovis */
  rc = init_clovis();
  if (rc < 0) {
      s3_log(S3_LOG_FATAL, "clovis_init failed!\n");
      s3daemon.delete_pidfile();
      return rc;
  }

  rc = create_s3_user_index(ACCOUNT_USER_INDEX_NAME);
  if (rc < 0) {
    s3_log(S3_LOG_FATAL, "Failed to create a KVS index\n");
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
  s3_log(S3_LOG_INFO, "Starting S3 listener on host = %s and port = %d!\n", bind_addr, bind_port);
  evhtp_bind_socket(htp, bind_addr, bind_port, 1024);
  rc = event_base_loop(global_evbase_handle, 0);
  if( rc == 0) {
    s3_log(S3_LOG_DEBUG, "Event base loop exited normally\n");
  } else {
    s3_log(S3_LOG_ERROR, "Event base loop exited due to unhandled exception in libevent's backend\n");
  }
  evhtp_unbind_socket(htp);
  evhtp_free(htp);
  event_base_free(global_evbase_handle);

  /* Clean-up */
  fini_clovis();
  if(option_instance->s3_performance_enabled()) {
    S3PerfLogger::finalize();
  }

  delete router;
  S3Option::destroy_instance();
  S3ErrorMessages::finalize();
  s3_log(S3_LOG_DEBUG, "S3server exiting...\n");
  s3daemon.delete_pidfile();
  fini_log();
  finalize_cli_options();

  return 0;
}
