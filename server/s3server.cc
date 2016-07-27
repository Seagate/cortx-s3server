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
#include "s3_option.h"
#include "s3_daemonize_server.h"

#define WEBSTORE "/home/seagate/webstore"

/* Program options */
#include <unistd.h>

const char *log_level_str[S3_LOG_DEBUG] = {"FATAL", "ERROR", "WARN", "INFO", "DEBUG"};

FILE *fp_log;
int s3log_level = S3_LOG_INFO;
evbase_t * global_evbase_handle;


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


int
main(int argc, char ** argv) {
  int rc = 0;
  const char  *bind_addr;
  uint16_t     bind_port;

  if (parse_and_load_config_options(argc, argv) < 0) {
      exit(1);
  }

  // Load Any configs.
  S3ErrorMessages::init_messages();

  S3Option * option_instance = S3Option::get_instance();
  if(option_instance->get_log_level() != "") {
    if(option_instance->get_log_level() == "INFO") {
      s3log_level = S3_LOG_INFO;
    } else if(option_instance->get_log_level() == "DEBUG") {
      s3log_level = S3_LOG_DEBUG;
    } else if(option_instance->get_log_level() == "ERROR") {
      s3log_level = S3_LOG_ERROR;
    } else if(option_instance->get_log_level() == "FATAL") {
      s3log_level = S3_LOG_FATAL;
    } else if(option_instance->get_log_level() == "WARN") {
      s3log_level = S3_LOG_WARN;
    }
  } else {
    s3log_level = S3_LOG_INFO;
  }

  if(option_instance->get_log_filename() != "") {
    fp_log = std::fopen(option_instance->get_log_filename().c_str(), "a");
    if(fp_log == NULL) {
      printf("File opening of log %s failed\n", option_instance->get_log_filename().c_str());
      return -1;
    }
  } else {
    fp_log = stdout;
  }

  option_instance->dump_options();
    // Initilise loggers
  if (option_instance->s3_performance_enabled()) {
    S3PerfLogger::initialize(option_instance->get_perf_log_filename());
  }

  S3Daemonize s3daemon;
  s3daemon.daemonize();
  s3daemon.register_signals();

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
  s3_log(S3_LOG_DEBUG, "S3server exiting...\n");
  s3daemon.delete_pidfile();
  if(fp_log != NULL && fp_log != stdout) {
    std::fclose(fp_log);
  }
  return 0;
}
