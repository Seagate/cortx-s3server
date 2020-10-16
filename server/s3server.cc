/*
 * Copyright (c) 2020 Seagate Technology LLC and/or its Affiliates
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * For any questions about this software or licensing,
 * please email opensource@seagate.com or cortx-questions@seagate.com.
 *
 */

#include <tuple>
#include <vector>
#include <set>
#include <sstream>

#include <openssl/md5.h>
#include <event2/thread.h>
#include <sys/resource.h>
#include <unistd.h>

#include "motr_helpers.h"
#include "evhtp_wrapper.h"
#include "fid/fid.h"
#include "murmur3_hash.h"
#include "s3_motr_layout.h"
#include "s3_common_utilities.h"
#include "s3_daemonize_server.h"
#include "s3_error_codes.h"
#include "s3_fi_common.h"
#include "s3_log.h"
#include "s3_mem_pool_manager.h"
#include "s3_option.h"
#include "s3_perf_logger.h"
#include "s3_request_object.h"
#include "s3_router.h"
#include "s3_stats.h"
#include "s3_timer.h"
#include "s3_uri_to_motr_oid.h"
#include "s3_audit_info.h"
#include "s3_audit_info_logger.h"
#include "s3_fake_motr_redis_kvs.h"
#include "s3_motr_wrapper.h"
#include "s3_m0_uint128_helper.h"
#include "s3_perf_metrics.h"
#include "s3_iem.h"

#define FOUR_KB 4096
// 32MB
#define MIN_RESERVE_SIZE 32768

#define WEBSTORE "/home/seagate/webstore"
#define MOTR_INIT_MAX_ALLOWED_TIME 15

/* Program options */
#include <unistd.h>

#define GLOBAL_BUCKET_LIST_INDEX_OID_U_LO 1
#define BUCKET_METADATA_LIST_INDEX_OID_U_LO 2
#define OBJECT_PROBABLE_DEAD_OID_LIST_INDEX_OID_U_LO 3
#define GLOBAL_INSTANCE_INDEX_U_LO 4

#define REPLICA_GLOBAL_BUCKET_LIST_INDEX_OID_U_LO 5
#define REPLICA_BUCKET_METADATA_LIST_INDEX_OID_U_LO 6

S3Option *g_option_instance = NULL;
evhtp_ssl_ctx_t *g_ssl_auth_ctx = NULL;
evbase_t *global_evbase_handle;
extern struct m0_realm motr_uber_realm;
// index will have bucket and account information
struct m0_uint128 global_bucket_list_index_oid;
// replica index of global_bucket_list_index_oid
struct m0_uint128 replica_global_bucket_list_index_oid;
// index will have bucket metada information
struct m0_uint128 bucket_metadata_list_index_oid;
// replica index of bucket_metadata_list_index_oid
struct m0_uint128 replica_bucket_metadata_list_index_oid;
// index will have s3server instance information
struct m0_uint128 global_instance_list_index;
// objects listed in this index are probable delete candidates and not absolute.
struct m0_uint128 global_probable_dead_object_list_index_oid;

int global_shutdown_in_progress;
pthread_t global_tid_indexop;
pthread_t global_tid_objop;

struct m0_uint128 global_instance_id;
int shutdown_motr_teardown_called;
std::set<struct s3_motr_op_context *> global_motr_object_ops_list;
std::set<struct s3_motr_idx_op_context *> global_motr_idx_ops_list;
std::set<struct s3_motr_idx_context *> global_motr_idx;
std::set<struct s3_motr_obj_context *> global_motr_obj;

void s3_motr_init_timeout_cb(evutil_socket_t fd, short event, void *arg) {
  s3_log(S3_LOG_ERROR, "", "Entering\n");
  s3_iem(LOG_ALERT, S3_IEM_MOTR_CONN_FAIL, S3_IEM_MOTR_CONN_FAIL_STR,
         S3_IEM_MOTR_CONN_FAIL_JSON);
  event_base_loopbreak(global_evbase_handle);
  return;
}

void *base_loop_thread(void *arg) {
  pthread_detach(pthread_self());
  event_base_loop(global_evbase_handle, EVLOOP_NO_EXIT_ON_EMPTY);
  return NULL;
}

extern "C" void s3_handler(evhtp_request_t *req, void *a) {
  // placeholder, required to complete the request processing.
  s3_log(S3_LOG_DEBUG, "", "Request Completed.\n");
}

static void on_client_request_error(evhtp_request_t *p_evhtp_req,
                                    evhtp_error_flags errtype, void *arg) {
  s3_log(S3_LOG_INFO, nullptr, "S3 Client disconnected: %s\n",
         S3CommonUtilities::evhtp_error_flags_description(errtype).c_str());
  if (p_evhtp_req) {
    RequestObject *p_s3_req = static_cast<RequestObject *>(p_evhtp_req->cbarg);
    if (p_s3_req) {
      p_s3_req->client_has_disconnected();
      p_evhtp_req->cbarg = nullptr;
    }
  }
}

//
// As per document http://www.wangafu.net/~nickm/libevent-book/Ref4_event.html
// Its safe to call all functions from the callback as
// libevent run in the event loop after the signal occurs
//
static void s3_signal_cb(evutil_socket_t sig, short events, void *user_data) {
  s3_log(S3_LOG_INFO, "", "Entering\n");
  s3_log(S3_LOG_INFO, "", "About to trigger shutdown\n");
  s3_kickoff_graceful_shutdown(1);
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
  return;
}

void set_fatal_handler_exit() { s3_fatal_handler = exit; }

void set_graceful_handler() { s3_fatal_handler = s3_kickoff_graceful_shutdown; }

extern "C" int log_http_header(evhtp_header_t *header, void *arg) {
  s3_log(S3_LOG_DEBUG, "", "http header(key = '%s', val = '%s')\n", header->key,
         header->val);
  return 0;
}

static evhtp_res on_client_request_fini(evhtp_request_t *p_evhtp_req,
                                        void *arg) {
  s3_log(S3_LOG_DEBUG, "", "Finalize s3 client request(%p)\n", p_evhtp_req);
  // Around this event libevhtp will free request, so we
  // protect S3 code from accessing freed request
  if (p_evhtp_req) {
    RequestObject *p_s3_rq = static_cast<RequestObject *>(p_evhtp_req->cbarg);
    s3_log(S3_LOG_DEBUG, "", "RequestObject(%p)\n", p_s3_rq);
    if (p_s3_rq) {
      p_s3_rq->client_has_disconnected();
      p_evhtp_req->cbarg = nullptr;
    }
  }
  return EVHTP_RES_OK;
}

extern "C" evhtp_res dispatch_s3_api_request(evhtp_request_t *req,
                                             evhtp_headers_t *hdrs, void *arg) {
  s3_log(S3_LOG_INFO, "", "Received Request with uri [%s].\n",
         req->uri->path->full);

  if (req->uri->query_raw) {
    s3_log(S3_LOG_DEBUG, "", "Received Request with query params [%s].\n",
           req->uri->query_raw);
  }
  // Log http headers
  evhtp_headers_for_each(hdrs, log_http_header, NULL);

  Router *router = static_cast<Router *>(arg);

  EvhtpInterface *evhtp_obj_ptr = new EvhtpWrapper();
  EventInterface *event_obj_ptr = new EventWrapper();
  std::shared_ptr<S3RequestObject> s3_request =
      std::make_shared<S3RequestObject>(req, evhtp_obj_ptr, nullptr,
                                        event_obj_ptr);

  // validate content length against out of range and invalid argument
  if (!s3_request->validate_content_length()) {
    s3_request->pause();
    evhtp_unset_all_hooks(&req->conn->hooks);
    // Send response with 'Bad Request' code.
    s3_log(
        S3_LOG_DEBUG, "",
        "sending 'Bad Request' response to client due to invalid request...\n");
    S3Error error("BadRequest", s3_request->get_request_id(), "");
    std::string &response_xml = error.to_xml();
    s3_request->set_out_header_value("Content-Type", "application/xml");
    s3_request->set_out_header_value("Content-Length",
                                     std::to_string(response_xml.length()));

    s3_request->send_response(error.get_http_status_code(), response_xml);
    return EVHTP_RES_OK;
  } else if (s3_request->is_header_present("content-md5") &&
             !s3_request->validate_content_md5()) {
    // Send HTTP 400 response, with S3 error 'InvalidDigest' to client
    s3_request->pause();
    evhtp_unset_all_hooks(&req->conn->hooks);
    s3_log(S3_LOG_DEBUG, "",
           "Sending 'Invalid Digest' response to client due to invalid md5 "
           "digest...\n");
    S3Error error("InvalidDigest", s3_request->get_request_id(), "");
    std::string &response_xml = error.to_xml();
    s3_request->set_out_header_value("Content-Type", "application/xml");
    s3_request->set_out_header_value("Content-Length",
                                     std::to_string(response_xml.length()));

    s3_request->send_response(error.get_http_status_code(), response_xml);
    return EVHTP_RES_OK;
  }

  // request validation is done and we are ready to use request
  // so initialise the s3 request;
  s3_request->initialise();

  if (S3Option::get_instance()->get_is_s3_shutting_down() &&
      !s3_fi_is_enabled("shutdown_system_tests_in_progress")) {
    // We are shutting down, so don't entertain new requests.
    s3_request->pause();
    evhtp_unset_all_hooks(&req->conn->hooks);
    // Send response with 'Service Unavailable' code.
    s3_log(S3_LOG_DEBUG, "", "sending 'Service Unavailable' response...\n");
    S3Error error("ServiceUnavailable", s3_request->get_request_id(), "");
    std::string &response_xml = error.to_xml();
    s3_request->set_out_header_value("Content-Type", "application/xml");
    s3_request->set_out_header_value("Content-Length",
                                     std::to_string(response_xml.length()));
    s3_request->set_out_header_value("Connection", "close");
    int shutdown_grace_period =
        S3Option::get_instance()->get_s3_grace_period_sec();
    s3_request->set_out_header_value("Retry-After",
                                     std::to_string(shutdown_grace_period));

    s3_request->send_response(error.get_http_status_code(), response_xml);
    return EVHTP_RES_OK;
  }

  struct pool_info poolinfo;
  int rc = event_mempool_getinfo(&poolinfo);
  if (rc != 0) {
    s3_log(S3_LOG_FATAL, "", "Issue with memory pool!\n");
  } else {
    s3_log(S3_LOG_DEBUG, "",
           "mempool info: mempool_item_size = %zu "
           "free_bufs_in_pool = %d "
           "number_of_bufs_shared = %d "
           "total_bufs_allocated_by_pool = %d\n",
           poolinfo.mempool_item_size, poolinfo.free_bufs_in_pool,
           poolinfo.number_of_bufs_shared,
           poolinfo.total_bufs_allocated_by_pool);
  }

  // Check if we have enough approx memory to proceed with request
  if (s3_request->get_api_type() == S3ApiType::object &&
      s3_request->http_verb() == S3HttpVerb::PUT) {
    int layout_id = S3MotrLayoutMap::get_instance()->get_layout_for_object_size(
        s3_request->get_data_length());
    if (!S3MemoryProfile().we_have_enough_memory_for_put_obj(layout_id) ||
        !S3MemoryProfile().free_memory_in_pool_above_threshold_limits()) {
      s3_log(S3_LOG_DEBUG, s3_request->get_request_id().c_str(),
             "Limited memory: Rejecting PUT object/part request with retry.\n");
      s3_request->respond_retry_after(1);
      return EVHTP_RES_OK;
    } else if (req->buffer_out) {
      // Reserve memory for an error response
      evbuffer_expand(req->buffer_out, 4096);
    }
  }
  req->cbarg = static_cast<RequestObject *>(s3_request.get());

  evhtp_set_hook(&req->hooks, evhtp_hook_on_error,
                 (evhtp_hook)on_client_request_error, NULL);
  evhtp_set_hook(&req->hooks, evhtp_hook_on_request_fini,
                 (evhtp_hook)on_client_request_fini, NULL);

  router->dispatch(s3_request);

  auto buffered_input = s3_request->get_buffered_input();

  if (buffered_input && !buffered_input->is_freezed()) {
    s3_request->set_start_client_request_read_timeout();
  }

  return EVHTP_RES_OK;
}

extern "C" evhtp_res dispatch_motr_api_request(evhtp_request_t *req,
                                               evhtp_headers_t *hdrs,
                                               void *arg) {
  s3_log(S3_LOG_INFO, "", "Received Request with uri [%s].\n",
         req->uri->path->full);

  if (req->uri->query_raw) {
    s3_log(S3_LOG_DEBUG, "", "Received Request with query params [%s].\n",
           req->uri->query_raw);
  }
  // Log http headers
  evhtp_headers_for_each(hdrs, log_http_header, NULL);

  Router *motr_router = static_cast<Router *>(arg);

  EvhtpInterface *evhtp_obj_ptr = new EvhtpWrapper();
  EventInterface *event_obj_ptr = new EventWrapper();
  std::shared_ptr<MotrRequestObject> motr_request =
      std::make_shared<MotrRequestObject>(req, evhtp_obj_ptr, nullptr,
                                          event_obj_ptr);

  // validate content length against out of range
  // and invalid argument
  if (!motr_request->validate_content_length()) {
    motr_request->pause();
    evhtp_unset_all_hooks(&req->conn->hooks);
    // Send response with 'Bad Request' code.
    s3_log(
        S3_LOG_DEBUG, "",
        "sending 'Bad Request' response to client due to invalid request...\n");
    S3Error error("BadRequest", motr_request->get_request_id(), "");
    std::string &response_xml = error.to_xml();
    motr_request->set_out_header_value("Content-Type", "application/xml");
    motr_request->set_out_header_value("Content-Length",
                                       std::to_string(response_xml.length()));

    motr_request->send_response(error.get_http_status_code(), response_xml);
    return EVHTP_RES_OK;
  }

  // request validation is done and we are ready to use request
  // so initialise the s3 request;
  motr_request->initialise();

  if (S3Option::get_instance()->get_is_s3_shutting_down() &&
      !s3_fi_is_enabled("shutdown_system_tests_in_progress")) {
    // We are shutting down, so don't entertain new requests.
    motr_request->pause();
    evhtp_unset_all_hooks(&req->conn->hooks);
    // Send response with 'Service Unavailable' code.
    s3_log(S3_LOG_DEBUG, "", "sending 'Service Unavailable' response...\n");
    S3Error error("ServiceUnavailable", motr_request->get_request_id(), "");
    std::string &response_xml = error.to_xml();
    motr_request->set_out_header_value("Content-Type", "application/xml");
    motr_request->set_out_header_value("Content-Length",
                                       std::to_string(response_xml.length()));
    motr_request->set_out_header_value("Connection", "close");
    int shutdown_grace_period =
        S3Option::get_instance()->get_s3_grace_period_sec();
    motr_request->set_out_header_value("Retry-After",
                                       std::to_string(shutdown_grace_period));
    motr_request->send_response(error.get_http_status_code(), response_xml);
    return EVHTP_RES_OK;
  }

  req->cbarg = static_cast<RequestObject *>(motr_request.get());

  evhtp_set_hook(&req->hooks, evhtp_hook_on_error,
                 (evhtp_hook)on_client_request_error, NULL);
  evhtp_set_hook(&req->hooks, evhtp_hook_on_request_fini,
                 (evhtp_hook)on_client_request_fini, NULL);

  motr_router->dispatch(motr_request);
  if (!motr_request->get_buffered_input()->is_freezed()) {
    motr_request->set_start_client_request_read_timeout();
  }

  return EVHTP_RES_OK;
}

static evhtp_res process_request_data(evhtp_request_t *p_evhtp_req,
                                      evbuf_t *buf, void *arg) {
  RequestObject *p_s3_req = static_cast<RequestObject *>(p_evhtp_req->cbarg);
  const char *psz_request_id =
      p_s3_req ? p_s3_req->get_request_id().c_str() : nullptr;

  s3_log(S3_LOG_DEBUG, psz_request_id, "RequestObject(%p)\n", p_s3_req);

  if (p_s3_req) {
    // Data has arrived so disable read timeout
    p_s3_req->stop_client_read_timer();

    s3_log(S3_LOG_DEBUG, psz_request_id,
           "Received Request body %zu bytes for sock = %d\n",
           evbuffer_get_length(buf), p_evhtp_req->conn->sock);

    if (!p_s3_req->is_incoming_data_ignored()) {
      evbuf_t *s3_buf = evbuffer_new();
      evbuffer_add_buffer(s3_buf, buf);

      p_s3_req->notify_incoming_data(s3_buf);

      return EVHTP_RES_OK;
    }
  }
  if (buf) {
    s3_log(S3_LOG_DEBUG, psz_request_id, "Drain incoming data\n");
    evbuffer_drain(buf, -1);
  }
  return EVHTP_RES_OK;
}

extern "C" evhtp_res process_motr_api_request_data(evhtp_request_t *req,
                                                   evbuf_t *buf, void *arg) {
  RequestObject *request = static_cast<RequestObject *>(req->cbarg);
  s3_log(S3_LOG_DEBUG, "", "MotrRequestObject(%p)\n", request);
  if (request && !request->is_incoming_data_ignored()) {
    s3_log(S3_LOG_DEBUG, request->get_request_id().c_str(),
           "Received Request body %zu bytes for sock = %d\n",
           evbuffer_get_length(buf), req->conn->sock);
    // Data has arrived so disable read timeout
    request->stop_client_read_timer();
    evbuf_t *s3_buf = evbuffer_new();
    evbuffer_add_buffer(s3_buf, buf);

    request->notify_incoming_data(s3_buf);

  } else {
    if (request) {
      request->stop_client_read_timer();
    }
    evhtp_unset_all_hooks(&req->conn->hooks);
    evhtp_unset_all_hooks(&req->hooks);
    s3_log(S3_LOG_DEBUG, "",
           "Motr request failed, Ignoring data for this request \n");
  }

  return EVHTP_RES_OK;
}

extern "C" evhtp_res set_s3_connection_handlers(evhtp_connection_t *conn,
                                                void *arg) {
  evhtp_set_hook(&conn->hooks, evhtp_hook_on_headers,
                 (evhtp_hook)dispatch_s3_api_request, arg);
  evhtp_set_hook(&conn->hooks, evhtp_hook_on_read,
                 (evhtp_hook)process_request_data, NULL);
  return EVHTP_RES_OK;
}

extern "C" evhtp_res set_motr_http_api_connection_handlers(
    evhtp_connection_t *conn, void *arg) {
  evhtp_set_hook(&conn->hooks, evhtp_hook_on_headers,
                 (evhtp_hook)dispatch_motr_api_request, arg);
  evhtp_set_hook(&conn->hooks, evhtp_hook_on_read,
                 (evhtp_hook)process_motr_api_request_data, NULL);
  return EVHTP_RES_OK;
}

void fatal_libevent(int err) {
  s3_log(S3_LOG_ERROR, "", "Fatal error occured in libevent, error = %d\n",
         err);
}

// This function will initialize global variable, should not be removed
void init_s3_index_oid(struct m0_uint128 &global_index_oid,
                       const uint64_t &u_lo_index_offset) {
  struct m0_uint128 temp = {0ULL, 0ULL};
  temp.u_lo = u_lo_index_offset;
  // reserving an oid for global index -- M0_ID_APP + u_lo_index_offset
  m0_uint128_add(&global_index_oid, &M0_ID_APP, &temp);
  struct m0_fid index_fid =
      M0_FID_TINIT('x', global_index_oid.u_hi, global_index_oid.u_lo);
  global_index_oid.u_hi = index_fid.f_container;
  global_index_oid.u_lo = index_fid.f_key;
}

bool init_ssl(evhtp_t *htp) {
  // Sample Code: libevhtp/examples/test.c
  char *cert_file =
      const_cast<char *>(g_option_instance->get_s3server_ssl_cert_file());
  char *pem_file =
      const_cast<char *>(g_option_instance->get_s3server_ssl_pem_file());
  evhtp_ssl_cfg_t scfg;
  memset(&scfg, '\0', sizeof(scfg));
  scfg.pemfile = pem_file;
  scfg.privfile = pem_file;
  scfg.cafile = cert_file;
  // SSL session timeout in seconds
  scfg.ssl_ctx_timeout = g_option_instance->get_s3server_ssl_session_timeout();
  scfg.x509_verify_cb = NULL;
  scfg.x509_chk_issued_cb = NULL;

  if (evhtp_ssl_init(htp, &scfg) != 0) {
    s3_log(S3_LOG_ERROR, "", "evhtp_ssl_init failed\n");
    return false;
  }
  return true;
}

bool init_auth_ssl() {
  const char *cert_file = g_option_instance->get_iam_cert_file();
  SSL_library_init();
  ERR_load_crypto_strings();
  SSL_load_error_strings();
  g_ssl_auth_ctx = SSL_CTX_new(SSLv23_method());
  if (!SSL_CTX_load_verify_locations(g_ssl_auth_ctx, cert_file, NULL)) {
    s3_log(S3_LOG_ERROR, "", "SSL_CTX_load_verify_locations\n");
    return false;
  }
  SSL_CTX_set_verify(g_ssl_auth_ctx, SSL_VERIFY_PEER, NULL);
  SSL_CTX_set_verify_depth(g_ssl_auth_ctx, 1);
  return true;
}

void fini_auth_ssl() {
  if (g_ssl_auth_ctx) {
    SSL_CTX_free(g_ssl_auth_ctx);
  }

  // see Cleanup section in
  // https://wiki.openssl.org/index.php/Library_Initialization
  ERR_remove_state(0);
  ERR_free_strings();
  EVP_cleanup();
  CRYPTO_cleanup_all_ex_data();
  SSL_COMP_free_compression_methods();
}

// This index will be holding the ids for the bucket
int create_global_index(struct m0_uint128 &root_index_oid,
                        const uint64_t &u_lo_index_offset) {
  int rc;

  const auto motr_op_wait_period = g_option_instance->get_motr_op_wait_period();
  unsigned n_retry = g_option_instance->get_max_retry_count() + 1;

  // reserving an oid for root index -- M0_ID_APP + offset
  const auto m0_timeout = m0_time_from_now(motr_op_wait_period, 0);
  init_s3_index_oid(root_index_oid, u_lo_index_offset);

  struct m0_idx idx;
  memset(&idx, 0, sizeof idx);

  m0_idx_init(&idx, &motr_uber_realm, &root_index_oid);

  for (; --n_retry > 0; ::sleep(1)) {  // suspend current thread for 1 second
    struct m0_op *cr_op = nullptr;

    m0_entity_create(NULL, &idx.in_entity, &cr_op);
    m0_op_launch(&cr_op, 1);

    rc = m0_op_wait(cr_op, M0_BITS(M0_OS_FAILED, M0_OS_STABLE), m0_timeout);
    if (rc >= 0) {
      rc = m0_rc(cr_op);
    }
    teardown_motr_cancel_wait_op(cr_op, m0_timeout);

    if (-EEXIST == rc) {
      rc = 0;
      break;
    }
    if (-ETIMEDOUT == rc) {
      continue;
    }
    if (rc) {
      break;
    }
    struct m0_op *sync_op = nullptr;

    rc = m0_sync_op_init(&sync_op);
    if (rc) {
      break;
    }
    rc = m0_sync_entity_add(sync_op, &idx.in_entity);

    if (!rc) {
      m0_op_launch(&sync_op, 1);
      rc = m0_op_wait(sync_op, M0_BITS(M0_OS_FAILED, M0_OS_STABLE), m0_timeout);
    }
    teardown_motr_cancel_wait_op(sync_op, m0_timeout);

    if (rc != -ETIMEDOUT) {
      break;
    }
  }
  if (idx.in_entity.en_sm.sm_state) {
    m0_idx_fini(&idx);
  }
  if (rc) {
    s3_iem(LOG_ALERT, S3_IEM_MOTR_CONN_FAIL, S3_IEM_MOTR_CONN_FAIL_STR,
           S3_IEM_MOTR_CONN_FAIL_JSON);
  }
  return rc;
}

int create_global_replica_index(struct m0_uint128 &root_index_oid,
                                const uint64_t &u_lo_index_offset) {
  return create_global_index(root_index_oid, u_lo_index_offset);
}

void log_resource_limits() {
  int rc;
  struct rlimit rlimit;
  rc = getrlimit(RLIMIT_NOFILE, &rlimit);
  if (rc == 0) {
    s3_log(S3_LOG_DEBUG, "", "Open file limits: soft = %ld hard = %ld\n",
           rlimit.rlim_cur, rlimit.rlim_max);
  } else {
    s3_log(S3_LOG_ERROR, "", "getrlimit failed to set option RLIMIT_NOFILE\n");
  }
  rc = getrlimit(RLIMIT_CORE, &rlimit);
  if (rc == 0) {
    s3_log(S3_LOG_DEBUG, "", "Core file size limits: soft = %ld hard = %ld\n",
           rlimit.rlim_cur, rlimit.rlim_max);
  } else {
    s3_log(S3_LOG_WARN, "",
           "getrlimit call failed to set option RLIMIT_CORE\n");
  }
}

evhtp_t *create_evhtp_handle(evbase_t *evbase_handle, Router *router,
                             void *arg) {
  evhtp_t *htp = evhtp_new(global_evbase_handle, NULL);
#if defined SO_REUSEPORT
  if (g_option_instance->is_s3_reuseport_enabled()) {
    htp->enable_reuseport = 1;
  }
#else
  if (g_option_instance->is_s3_reuseport_enabled()) {
    s3_log(
        S3_LOG_ERROR,
        "Option --reuseport is true however OS Doesn't support SO_REUSEPORT\n");
    return NULL;
  }
#endif

  // So we can support queries like s3.com/bucketname?location or ?acl
  // So we can support empty queries (for s3fs) like s3.com/bucketname?prefix=
  evhtp_set_parser_flags(htp, EVHTP_PARSE_QUERY_FLAG_ALLOW_NULL_VALS |
                                  EVHTP_PARSE_QUERY_FLAG_ALLOW_EMPTY_VALS);

  // Main request processing (processing headers & body) is done in hooks
  evhtp_set_post_accept_cb(htp, set_s3_connection_handlers, router);

  // This handler is just like complete the request processing & respond
  evhtp_set_gencb(htp, s3_handler, NULL);
  return htp;
}

evhtp_t *create_evhtp_handle_for_motr(evbase_t *evbase_handle,
                                      Router *motr_router, void *arg) {
  evhtp_t *htp = evhtp_new(global_evbase_handle, NULL);
#if defined SO_REUSEPORT
  if (g_option_instance->is_motr_http_reuseport_enabled()) {
    htp->enable_reuseport = 1;
  }
#else
  if (g_option_instance->is_motr_http_reuseport_enabled()) {
    s3_log(
        S3_LOG_ERROR,
        "Option --reuseport is true however OS Doesn't support SO_REUSEPORT\n");
    return NULL;
  }
#endif

  // So we can support queries like s3.com/bucketname?location or ?acl
  // So we can support empty queries (for s3fs) like s3.com/bucketname?prefix=
  evhtp_set_parser_flags(htp, EVHTP_PARSE_QUERY_FLAG_ALLOW_NULL_VALS |
                                  EVHTP_PARSE_QUERY_FLAG_ALLOW_EMPTY_VALS);

  // Main request processing (processing headers & body) is done in hooks
  evhtp_set_post_accept_cb(htp, set_motr_http_api_connection_handlers,
                           motr_router);

  // This handler is just like complete the request processing & respond
  evhtp_set_gencb(htp, s3_handler, NULL);
  return htp;
}

void free_evhtp_handle(evhtp_t *htp) {
  if (htp) {
    evhtp_unbind_socket(htp);
    evhtp_free(htp);
  }
}

int main(int argc, char **argv) {
  int rc = 0;
  pthread_t tid;
  struct event *signal_sigint_event;
  struct event *signal_sigterm_event;
  // map will have s3server { fid, instance_id } information
  std::map<std::string, std::string> s3server_instance_id;

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
    // To satisfy valgrind
    finalize_cli_options();
    exit(1);
  }

  S3ErrorMessages::init_messages();
  g_option_instance = S3Option::get_instance();

  std::unique_ptr<S3PerfLogger> s3_perf_logger;
  // Init perf logger
  if (g_option_instance->s3_performance_enabled()) {

    s3_perf_logger.reset(
        new S3PerfLogger(g_option_instance->get_perf_log_filename()));

    if (!S3PerfLogger::is_enabled()) {
      s3_log(S3_LOG_ERROR, "", "Initialization of performance logger failed\n");
      exit(1);
    }
  }

  S3Daemonize s3daemon;
  set_fatal_handler_exit();
  s3daemon.daemonize();
#if 0
  s3daemon.register_signals();
#endif
  // dump the config
  g_option_instance->dump_options();

  if (g_option_instance->get_libevent_pool_max_threshold() < MIN_RESERVE_SIZE) {
    s3_log(
        S3_LOG_FATAL, "",
        "S3_LIBEVENT_POOL_RESERVE_SIZE in s3config.yaml cannot be less than %d",
        MIN_RESERVE_SIZE);
  }

  S3MotrLayoutMap::get_instance()->load_layout_recommendations(
      g_option_instance->get_layout_recommendation_file());

  // Init stats
  rc = s3_stats_init();
  if (rc < 0) {
    s3daemon.delete_pidfile();
    finalize_cli_options();
    s3_log(S3_LOG_FATAL, "", "Stats Init failed!!\n");
  }

  int libevent_mempool_flags = CREATE_ALIGNED_MEMORY;
  if (g_option_instance->get_libevent_mempool_zeroed_buffer()) {
    libevent_mempool_flags = libevent_mempool_flags | ZEROED_BUFFER;
  }

  // Call this function at starting as we need to make use of our own
  // memory allocation/deallocation functions
  rc = event_use_mempool(g_option_instance->get_libevent_pool_buffer_size(),
                         g_option_instance->get_libevent_pool_initial_size(),
                         g_option_instance->get_libevent_pool_expandable_size(),
                         g_option_instance->get_libevent_pool_max_threshold(),
                         libevent_mempool_flags);

  if (rc != 0) {
    s3daemon.delete_pidfile();
    finalize_cli_options();
    s3_log(S3_LOG_FATAL, "", "Memory pool creation for libevent failed!\n");
  }

  event_set_max_read(g_option_instance->get_libevent_max_read_size());
  evhtp_set_low_watermark(g_option_instance->get_libevent_max_read_size());

  // Call this function before creating event base
  evthread_use_pthreads();

  // Uncomment below apis if we want to run libevent in debug mode
  // event_enable_debug_logging(EVENT_DBG_ALL);
  // event_enable_debug_mode();

  global_evbase_handle = event_base_new();
  g_option_instance->set_eventbase(global_evbase_handle);

  if (evthread_make_base_notifiable(global_evbase_handle) < 0) {
    s3daemon.delete_pidfile();
    s3_log(S3_LOG_ERROR, "", "Couldn't make base notifiable!");
    finalize_cli_options();
    return 1;
  }

  if (S3AuditInfoLogger::init() != 0) {
    s3daemon.delete_pidfile();
    finalize_cli_options();
    s3_log(S3_LOG_FATAL, "", "Couldn't init audit logger!");
  }

  event_set_fatal_callback(fatal_libevent);
  if (g_option_instance->is_s3_ssl_auth_enabled()) {
    if (!init_auth_ssl()) {
      s3daemon.delete_pidfile();
      finalize_cli_options();
      s3_log(S3_LOG_FATAL, "",
             "SSL initialization for communication with Auth server failed!\n");
    }
  }

  Router *s3_router =
      new S3Router(new S3APIHandlerFactory(), new S3UriFactory());

  Router *motr_router =
      new MotrRouter(new MotrAPIHandlerFactory(), new MotrUriFactory());

  std::string ipv4_bind_addr;
  std::string ipv6_bind_addr;
  std::string motr_addr = g_option_instance->get_motr_http_bind_addr();
  uint16_t motr_http_bind_port = g_option_instance->get_motr_http_bind_port();
  uint16_t bind_port;

  evhtp_t *htp_ipv4 = NULL;
  evhtp_t *htp_ipv6 = NULL;

  evhtp_t *htp_motr = NULL;

  bind_port = g_option_instance->get_s3_bind_port();
  ipv4_bind_addr = g_option_instance->get_ipv4_bind_addr();
  ipv6_bind_addr = g_option_instance->get_ipv6_bind_addr();

  if (!ipv4_bind_addr.empty()) {
    htp_ipv4 = create_evhtp_handle(global_evbase_handle, s3_router, NULL);
    if (htp_ipv4 == NULL) {
      s3daemon.delete_pidfile();
      fini_log();
      finalize_cli_options();
      return 1;
    }
  }

  if (!ipv6_bind_addr.empty()) {
    htp_ipv6 = create_evhtp_handle(global_evbase_handle, s3_router, NULL);
    if (htp_ipv6 == NULL) {
      s3daemon.delete_pidfile();
      fini_log();
      finalize_cli_options();
      return 1;
    }
  }

  if (!motr_addr.empty()) {
    htp_motr =
        create_evhtp_handle_for_motr(global_evbase_handle, motr_router, NULL);
    if (htp_motr == NULL) {
      s3daemon.delete_pidfile();
      fini_log();
      finalize_cli_options();
      return 1;
    }
  }

  if (g_option_instance->is_s3server_ssl_enabled()) {
    if (htp_ipv4 != NULL) {
      if (!init_ssl(htp_ipv4)) {
        s3daemon.delete_pidfile();
        finalize_cli_options();
        s3_log(S3_LOG_FATAL, "",
               "SSL initialization failed for s3server for IPV4!\n");
      }
    }
    if (htp_ipv6 != NULL) {
      if (!init_ssl(htp_ipv6)) {
        s3daemon.delete_pidfile();
        finalize_cli_options();
        s3_log(S3_LOG_FATAL, "",
               "SSL initialization failed for s3server for IPV6!\n");
      }
    }
  }

  // This thread is created to do event base looping
  // Its to send IEM message in case of motr initialization hang
  rc = pthread_create(&tid, NULL, &base_loop_thread, NULL);
  if (rc != 0) {
    s3daemon.delete_pidfile();
    finalize_cli_options();
    s3_log(S3_LOG_FATAL, "", "Failed to create pthread\n");
  }

  int motr_read_mempool_flags = CREATE_ALIGNED_MEMORY;
  if (g_option_instance->get_motr_read_mempool_zeroed_buffer()) {
    motr_read_mempool_flags = motr_read_mempool_flags | ZEROED_BUFFER;
  }

  // Create memory pool for motr read operations.
  rc = S3MempoolManager::create_pool(
      g_option_instance->get_motr_read_pool_max_threshold(),
      g_option_instance->get_motr_unit_sizes_for_mem_pool(),
      g_option_instance->get_motr_read_pool_initial_buffer_count(),
      g_option_instance->get_motr_read_pool_expandable_count(),
      motr_read_mempool_flags);

  if (rc != 0) {
    s3daemon.delete_pidfile();
    fini_auth_ssl();
    finalize_cli_options();
    s3_log(S3_LOG_FATAL, "",
           "Memory pool creation for motr read buffers failed!\n");
  }

  log_resource_limits();
  struct timeval tv;
  tv.tv_usec = 0;
  tv.tv_sec = MOTR_INIT_MAX_ALLOWED_TIME;

  struct event *timer_event =
      event_new(global_evbase_handle, -1, 0, s3_motr_init_timeout_cb, NULL);

  // Register the timer event, in case if Motr initialization hangs, then in
  // callback
  // we will send IEM Alert, so that IO stack gets restarted
  event_add(timer_event, &tv);

  /* Initialise Motr */
  rc = init_motr();
  if (rc < 0) {
    s3daemon.delete_pidfile();
    fini_auth_ssl();
    finalize_cli_options();
    s3_log(S3_LOG_FATAL, "", "motr_init failed!\n");
  }
  // delete the timer event
  event_del(timer_event);
  event_free(timer_event);
  // Bail out of the event loop
  event_base_loopbreak(global_evbase_handle);

  // Init addb
  rc = s3_addb_init();
  if (rc < 0) {
    s3daemon.delete_pidfile();
    fini_auth_ssl();
    fini_motr();
    finalize_cli_options();
    s3_log(S3_LOG_FATAL, "", "S3 ADDB Init failed!\n");
  }

  // global_bucket_list_index_oid - will hold bucket name as key, its owner
  // account information and region as value.
  rc = create_global_index(global_bucket_list_index_oid,
                           GLOBAL_BUCKET_LIST_INDEX_OID_U_LO);
  if (rc < 0) {
    s3daemon.delete_pidfile();
    fini_auth_ssl();
    fini_motr();
    // fatal message will call exit
    s3_log(S3_LOG_FATAL, "", "Failed to create a global bucket KVS index\n");
  }

  // create replica copy of global_bucket_list_index_oid. It will be used
  // by s3 recovery tool to recover global_bucket_list_index_oid,
  // incase of metadata corruption.
  rc = create_global_replica_index(replica_global_bucket_list_index_oid,
                                   REPLICA_GLOBAL_BUCKET_LIST_INDEX_OID_U_LO);
  if (rc < 0) {
    s3daemon.delete_pidfile();
    fini_auth_ssl();
    fini_motr();
    // fatal message will call exit
    s3_log(S3_LOG_FATAL, "",
           "Failed to create replica for global bucket KVS index\n");
  }

  // bucket_metadata_list_index_oid - will hold accountid/bucket_name as key,
  // bucket medata as value.
  rc = create_global_index(bucket_metadata_list_index_oid,
                           BUCKET_METADATA_LIST_INDEX_OID_U_LO);
  if (rc < 0) {
    s3daemon.delete_pidfile();
    fini_auth_ssl();
    fini_motr();
    finalize_cli_options();
    s3_log(S3_LOG_FATAL, "", "Failed to create a bucket metadata KVS index\n");
  }

  // create replica copy of bucket_metadata_list_index_oid. It will be used
  // by s3 recovery tool to recover bucket_metadata_list_index_oid,
  // incase of metadata corruption.
  rc = create_global_replica_index(replica_bucket_metadata_list_index_oid,
                                   REPLICA_BUCKET_METADATA_LIST_INDEX_OID_U_LO);
  if (rc < 0) {
    s3daemon.delete_pidfile();
    fini_auth_ssl();
    fini_motr();
    // fatal message will call exit
    s3_log(S3_LOG_FATAL, "",
           "Failed to create replica for global bucket metadata index\n");
  }

  // global_probable_dead_object_list_index_oid - will have stale object oid
  // information
  rc = create_global_index(global_probable_dead_object_list_index_oid,
                           OBJECT_PROBABLE_DEAD_OID_LIST_INDEX_OID_U_LO);
  if (rc < 0) {
    s3daemon.delete_pidfile();
    fini_auth_ssl();
    fini_motr();
    finalize_cli_options();
    s3_log(S3_LOG_FATAL, "", "Failed to global object leak list KVS index\n");
  }

  // global_instance_list_index - will hold s3server fid as key,
  // instance id as value.
  rc = create_global_index(global_instance_list_index,
                           GLOBAL_INSTANCE_INDEX_U_LO);
  if (rc < 0) {
    s3daemon.delete_pidfile();
    fini_auth_ssl();
    fini_motr();
    finalize_cli_options();
    s3_log(S3_LOG_FATAL, "", "Failed to create global instance index\n");
  }

  extern struct m0_config motr_conf;

  std::string s3server_fid = motr_conf.mc_process_fid;
  s3_log(S3_LOG_INFO, "", "Process Fid= %s \n", s3server_fid.c_str());

  rc = create_new_instance_id(&global_instance_id);

  if (rc != 0) {
    s3daemon.delete_pidfile();
    fini_auth_ssl();
    fini_motr();
    finalize_cli_options();
    s3_log(S3_LOG_FATAL, "", "Failed to create unique instance id\n");
  }

  if (S3Option::get_instance()->is_sync_kvs_allowed()) {
    s3server_instance_id[s3server_fid] =
        S3M0Uint128Helper::to_string(global_instance_id);

    std::shared_ptr<S3MotrKVSWriterFactory> mote_kv_writer_factory;
    std::shared_ptr<S3MotrKVSWriter> motr_kv_writer;
    std::shared_ptr<MotrAPI> s3_motr_api;

    s3_motr_api = std::make_shared<ConcreteMotrAPI>();
    mote_kv_writer_factory = std::make_shared<S3MotrKVSWriterFactory>();

    if (!motr_kv_writer) {
      motr_kv_writer =
          mote_kv_writer_factory->create_sync_motr_kvs_writer("", s3_motr_api);
    }

    rc = motr_kv_writer->put_keyval_sync(global_instance_list_index,
                                         s3server_instance_id);
    if (rc != 0) {
      s3daemon.delete_pidfile();
      fini_auth_ssl();
      fini_motr();
      finalize_cli_options();
      s3_log(S3_LOG_FATAL, "",
             "Failed to add unique instance id to global index\n");
    }
  }

/* KD - setup for reading data */
/* set a callback to set per-connection hooks (via a post_accept cb) */
// evhtp_set_post_accept_cb(htp, set_my_connection_handlers, NULL);

#if 0
#ifndef EVHTP_DISABLE_EVTHR
    evhtp_use_threads(htp, NULL, 4, NULL);
#endif
#endif

  if (htp_ipv4) {
    // apend ipv4: prefix to identify by evhtp_bind_socket
    ipv4_bind_addr = "ipv4:" + ipv4_bind_addr;
    s3_log(S3_LOG_INFO, "",
           "Starting S3 listener on host = %s and port = %d!\n",
           ipv4_bind_addr.c_str(), bind_port);
    if ((rc = evhtp_bind_socket(htp_ipv4, ipv4_bind_addr.c_str(), bind_port,
                                1024)) < 0) {
      s3daemon.delete_pidfile();
      fini_auth_ssl();
      evhtp_free(htp_ipv4);
      fini_motr();
      finalize_cli_options();
      s3_log(S3_LOG_FATAL, "", "Could not bind socket: %s\n", strerror(errno));
    }
  }

  if (htp_ipv6) {
    // apend ipv6: prefix to identify by evhtp_bind_socket
    ipv6_bind_addr = "ipv6:" + ipv6_bind_addr;
    s3_log(S3_LOG_INFO, "",
           "Starting S3 listener on host = %s and port = %d!\n",
           ipv6_bind_addr.c_str(), bind_port);
    if ((rc = evhtp_bind_socket(htp_ipv6, ipv6_bind_addr.c_str(), bind_port,
                                1024)) < 0) {
      s3daemon.delete_pidfile();
      fini_auth_ssl();
      evhtp_free(htp_ipv6);
      fini_motr();
      finalize_cli_options();
      s3_log(S3_LOG_FATAL, "", "Could not bind socket: %s\n", strerror(errno));
    }
  }

  if (htp_motr) {
    // apend ipv4: prefix to identify by evhtp_bind_socket
    // ipv4_bind_addr = "ipv4:" + motr_addr;
    s3_log(S3_LOG_INFO, "",
           "Starting s3 motr_addr listener on host = %s and port = %d!\n",
           motr_addr.c_str(), motr_http_bind_port);
    if ((rc = evhtp_bind_socket(htp_motr, motr_addr.c_str(),
                                motr_http_bind_port, 1024)) < 0) {
      s3daemon.delete_pidfile();
      fini_auth_ssl();
      evhtp_free(htp_motr);
      fini_motr();
      finalize_cli_options();
      s3_log(S3_LOG_FATAL, "", "Could not bind socket: %s\n", strerror(errno));
    }
  }

  rc = s3_perf_metrics_init(global_evbase_handle);
  if (rc != 0) {
    s3daemon.delete_pidfile();
    fini_auth_ssl();
    evhtp_free(htp_motr);
    fini_motr();
    finalize_cli_options();
    s3_log(S3_LOG_FATAL, "", "Could not init perf metrics: %s\n",
           strerror(-rc));
  }

  signal_sigint_event = evsignal_new(global_evbase_handle, SIGINT, s3_signal_cb,
                                     (void *)global_evbase_handle);
  if (!signal_sigint_event || event_add(signal_sigint_event, NULL) < 0) {
    s3_log(S3_LOG_FATAL, "", "Could not create/add a signal SIGINT event!\n");
  }
  signal_sigterm_event =
      evsignal_new(global_evbase_handle, SIGTERM, s3_signal_cb,
                   (void *)global_evbase_handle);
  if (!signal_sigterm_event || event_add(signal_sigterm_event, NULL) < 0) {
    s3_log(S3_LOG_FATAL, "", "Could not create/add a signal SIGTERM event!\n");
  }

  /* Set the fatal handler to graceful shutdown*/
  set_graceful_handler();

  // new flag in Libevent 2.1
  // EVLOOP_NO_EXIT_ON_EMPTY tells event_base_loop()
  // to keep looping even when there are no pending events
  rc = event_base_loop(global_evbase_handle, EVLOOP_NO_EXIT_ON_EMPTY);
  if (rc == 0) {
    s3_log(S3_LOG_INFO, "",
           "Event base loop exited normally, Shutting down s3server\n");
  } else {
    s3_log(S3_LOG_ERROR, "",
           "Event base loop exited due to unhandled exception in libevent's "
           "backend\n");
  }

  shutdown_motr_teardown_called = 1;
  global_motr_teardown();
  s3_perf_metrics_fini();
  pthread_join(global_tid_indexop, NULL);
  pthread_join(global_tid_objop, NULL);
  S3FakeMotrRedisKvs::destroy_instance();
  free_evhtp_handle(htp_ipv4);
  free_evhtp_handle(htp_ipv6);
  free_evhtp_handle(htp_motr);

  fini_auth_ssl();

  /* Clean-up */
  fini_motr();

  delete s3_router;
  delete motr_router;
  S3ErrorMessages::finalize();
  s3_log(S3_LOG_DEBUG, "", "S3server exiting...\n");
  s3daemon.delete_pidfile();
  s3_stats_fini();
  S3AuditInfoLogger::finalize();
  finalize_cli_options();
  S3MempoolManager::destroy_instance();
  S3MotrLayoutMap::destroy_instance();
  S3Option::destroy_instance();
  event_destroy_mempool();
  fini_log();
  event_free(signal_sigint_event);
  event_free(signal_sigterm_event);
  event_base_free(global_evbase_handle);
  // free all globally held resources
  // so that leak-check tools dont complain
  // Added in libevent 2.1
  libevent_global_shutdown();
  return 0;
}
