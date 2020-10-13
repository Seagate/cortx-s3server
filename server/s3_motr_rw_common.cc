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

#include "s3_motr_rw_common.h"
#include "s3_asyncop_context_base.h"
#include "s3_iem.h"
#include "s3_log.h"
#include "s3_post_to_main_loop.h"
#include "s3_motr_kvs_reader.h"
#include "s3_motr_kvs_writer.h"
#include "s3_fake_motr_kvs.h"
#include "s3_common_utilities.h"

#include <ctime>

/*
 *  <IEM_INLINE_DOCUMENTATION>
 *    <event_code>047003001</event_code>
 *    <application>S3 Server</application>
 *    <submodule>Motr</submodule>
 *    <description>Motr connection failed</description>
 *    <audience>Service</audience>
 *    <details>
 *      Motr operation failed due to connection failure.
 *      The data section of the event has following keys:
 *        time - timestamp.
 *        node - node name.
 *        pid  - process-id of s3server instance, useful to identify logfile.
 *        file - source code filename.
 *        line - line number within file where error occurred.
 *    </details>
 *    <service_actions>
 *      Save the S3 server log files. Check status of Motr service, restart it
 *      if needed. Restart S3 server if needed.
 *      If problem persists, contact development team for further investigation.
 *    </service_actions>
 *  </IEM_INLINE_DOCUMENTATION>
 */

// To prevent motr failure in case of rapid ETIMEDOUT errors
// s3server should stop itself
// number of errors should not be more than
// S3_SERVER_MOTR_ETIMEDOUT_MAX_THRESHOLD in S3_SERVER_MOTR_ETIMEDOUT_WINDOW_SEC
// seconds
// Note: these vars are not static because they used as extern in UTs
int64_t gs_timeout_window_start = time(NULL);
unsigned gs_timeout_cnt = 0;
bool gs_timeout_shutdown_in_progress = false;
void (*gs_motr_timeout_shutdown)(int ignore) = s3_kickoff_graceful_shutdown;

// This is run on main thread.
void motr_op_done_on_main_thread(evutil_socket_t, short events,
                                 void *user_data) {
  std::string request_id;
  if (user_data == NULL) {
    s3_log(S3_LOG_DEBUG, "", "Entering\n");
    s3_log(S3_LOG_ERROR, "", "Input argument user_data is NULL\n");
    s3_log(S3_LOG_DEBUG, "", "Exiting\n");
    return;
  }
  struct user_event_context *user_context =
      (struct user_event_context *)user_data;
  S3AsyncOpContextBase *context = (S3AsyncOpContextBase *)user_context->app_ctx;
  if (context == NULL) {
    s3_log(S3_LOG_ERROR, "", "context pointer is NULL\n");
  }
  if (context->get_request()) {
    request_id = context->get_request()->get_request_id();
  }
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  struct event *s3user_event = (struct event *)user_context->user_event;
  if (s3user_event == NULL) {
    s3_log(S3_LOG_ERROR, request_id, "User event is NULL\n");
  }
  context->log_timer();

  if (context->is_at_least_one_op_successful()) {
    context->on_success_handler()();  // Invoke the handler.
  } else {
    int error_code = context->get_errno_for(0);
    if ((error_code == -ETIMEDOUT) || (error_code == -ESHUTDOWN) ||
        (error_code == -ECONNREFUSED) || (error_code == -EHOSTUNREACH) ||
        (error_code == -ENOTCONN) || (error_code == -ECANCELED)) {
      // fatal iem are genrated in motr as a result of appropriate action
      s3_iem(LOG_ERR, S3_IEM_MOTR_CONN_FAIL, S3_IEM_MOTR_CONN_FAIL_STR,
             S3_IEM_MOTR_CONN_FAIL_JSON);
    }
    context->on_failed_handler()();  // Invoke the handler.

    // Count ETIMEDOUT errors for any motr operations
    // if number of errors for the etimedout_window sec
    // greater than etimedout_max_threshold s3server should be
    // restarted
    if (error_code == -ETIMEDOUT && !gs_timeout_shutdown_in_progress) {

      S3Option *optinst = S3Option::get_instance();
      unsigned err_thr = optinst->get_motr_etimedout_max_threshold();
      unsigned err_wnd = optinst->get_motr_etimedout_window_sec();
      int64_t curtime = time(NULL);

      if (curtime - gs_timeout_window_start >= (int64_t)err_wnd) {
        s3_log(S3_LOG_DEBUG, request_id,
               "Reset motr ETIMEDOUT window; cur window sec %" PRId64 ";",
               curtime - gs_timeout_window_start);
        gs_timeout_cnt = 0;
      }
      gs_timeout_cnt++;
      gs_timeout_window_start = curtime;
      if (gs_timeout_cnt >= err_thr) {
        s3_log(
            S3_LOG_ERROR, request_id,
            "Shutdown. Motr ETIMEDOUT error cnt %u reached threshold value %u "
            "for %" PRId64 " sec with allowed window %u sec",
            gs_timeout_cnt, err_thr, curtime - gs_timeout_window_start,
            err_wnd);
        if (gs_motr_timeout_shutdown) {
          gs_motr_timeout_shutdown(0);
        }
        gs_timeout_shutdown_in_progress = true;
      }
    }
  }

  free(user_data);
  // Free user event
  if (s3user_event) event_free(s3user_event);
  s3_log(S3_LOG_DEBUG, request_id, "Exiting\n");
}

// Motr callbacks, run in motr thread
void s3_motr_op_stable(struct m0_op *op) {
  s3_log(S3_LOG_DEBUG, "", "Entering\n");
  struct s3_motr_context_obj *ctx = (struct s3_motr_context_obj *)op->op_datum;

  S3AsyncOpContextBase *app_ctx =
      (S3AsyncOpContextBase *)ctx->application_context;
  int motr_rc = app_ctx->get_motr_api()->motr_op_rc(op);
  std::string request_id = app_ctx->get_request()->get_request_id();
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  s3_log(S3_LOG_DEBUG, request_id, "Return code = %d op_code = %d\n", motr_rc,
         op->op_code);

  s3_log(S3_LOG_DEBUG, request_id, "op_index_in_launch = %d\n",
         ctx->op_index_in_launch);

  app_ctx->set_op_errno_for(ctx->op_index_in_launch, motr_rc);

  if (0 == motr_rc) {
    app_ctx->set_op_status_for(ctx->op_index_in_launch,
                               S3AsyncOpStatus::success, "Success.");
  } else {
    app_ctx->set_op_status_for(ctx->op_index_in_launch, S3AsyncOpStatus::failed,
                               "Operation Failed.");
  }
  free(ctx);
  if (app_ctx->incr_response_count() == app_ctx->get_ops_count()) {
    struct user_event_context *user_ctx = (struct user_event_context *)calloc(
        1, sizeof(struct user_event_context));
    user_ctx->app_ctx = app_ctx;
    app_ctx->stop_timer();

#ifdef S3_GOOGLE_TEST
    evutil_socket_t test_sock = 0;
    short events = 0;
    motr_op_done_on_main_thread(test_sock, events, (void *)user_ctx);
#else
    S3PostToMainLoop((void *)user_ctx)(motr_op_done_on_main_thread, request_id);
#endif  // S3_GOOGLE_TEST
  }
  s3_log(S3_LOG_DEBUG, request_id, "Exiting\n");
}

void s3_motr_op_failed(struct m0_op *op) {
  struct s3_motr_context_obj *ctx = (struct s3_motr_context_obj *)op->op_datum;

  S3AsyncOpContextBase *app_ctx =
      (S3AsyncOpContextBase *)ctx->application_context;
  std::string request_id = app_ctx->get_request()->get_request_id();
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  int motr_rc = app_ctx->get_motr_api()->motr_op_rc(op);
  s3_log(S3_LOG_ERROR, request_id, "Error code = %d\n", motr_rc);

  s3_log(S3_LOG_DEBUG, request_id, "op_index_in_launch = %d\n",
         ctx->op_index_in_launch);

  app_ctx->set_op_errno_for(ctx->op_index_in_launch, motr_rc);
  app_ctx->set_op_status_for(ctx->op_index_in_launch, S3AsyncOpStatus::failed,
                             "Operation Failed.");
  // If we faked failure reset motr internal code.
  if (ctx->is_fake_failure) {
    op->op_rc = 0;
    ctx->is_fake_failure = 0;
  }
  free(ctx);
  if (app_ctx->incr_response_count() == app_ctx->get_ops_count()) {
    struct user_event_context *user_ctx = (struct user_event_context *)calloc(
        1, sizeof(struct user_event_context));
    user_ctx->app_ctx = app_ctx;
    app_ctx->stop_timer(false);
#ifdef S3_GOOGLE_TEST
    evutil_socket_t test_sock = 0;
    short events = 0;
    motr_op_done_on_main_thread(test_sock, events, (void *)user_ctx);
#else
    S3PostToMainLoop((void *)user_ctx)(motr_op_done_on_main_thread, request_id);
#endif  // S3_GOOGLE_TEST
  }
  s3_log(S3_LOG_DEBUG, request_id, "Exiting\n");
}

void s3_motr_op_pre_launch_failure(void *application_context, int rc) {
  S3AsyncOpContextBase *app_ctx = (S3AsyncOpContextBase *)application_context;
  std::string request_id = app_ctx->get_request()->get_request_id();
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  s3_log(S3_LOG_DEBUG, request_id, "Error code = %d\n", rc);
  app_ctx->set_op_errno_for(0, rc);
  app_ctx->set_op_status_for(0, S3AsyncOpStatus::failed, "Operation Failed.");
  struct user_event_context *user_ctx =
      (struct user_event_context *)calloc(1, sizeof(struct user_event_context));
  user_ctx->app_ctx = app_ctx;
#ifdef S3_GOOGLE_TEST
  evutil_socket_t test_sock = 0;
  short events = 0;
  motr_op_done_on_main_thread(test_sock, events, (void *)user_ctx);
#else
  S3PostToMainLoop((void *)user_ctx)(motr_op_done_on_main_thread, request_id);
#endif  // S3_GOOGLE_TEST
  s3_log(S3_LOG_DEBUG, request_id, "Exiting\n");
}

void s3_motr_dummy_op_stable(evutil_socket_t, short events, void *user_data) {
  s3_log(S3_LOG_DEBUG, "", "Entering\n");
  struct user_event_context *user_context =
      (struct user_event_context *)user_data;
  struct m0_op *op = (struct m0_op *)user_context->app_ctx;
  // This can be mocked from GTest but system tests call this method too,
  // where m0_rc can't be mocked.
  op->op_rc = 0;  // fake success

  if (op->op_code == M0_IC_GET) {
    struct s3_motr_context_obj *ctx =
        (struct s3_motr_context_obj *)op->op_datum;

    S3MotrKVSReaderContext *read_ctx =
        (S3MotrKVSReaderContext *)ctx->application_context;

    op->op_rc = S3FakeMotrKvs::instance()->kv_read(
        op->op_entity->en_id, *read_ctx->get_motr_kvs_op_ctx());
  } else if (M0_IC_NEXT == op->op_code) {
    struct s3_motr_context_obj *ctx =
        (struct s3_motr_context_obj *)op->op_datum;

    S3MotrKVSReaderContext *read_ctx =
        (S3MotrKVSReaderContext *)ctx->application_context;

    op->op_rc = S3FakeMotrKvs::instance()->kv_next(
        op->op_entity->en_id, *read_ctx->get_motr_kvs_op_ctx());
  } else if (M0_IC_PUT == op->op_code) {
    struct s3_motr_context_obj *ctx =
        (struct s3_motr_context_obj *)op->op_datum;

    S3AsyncMotrKVSWriterContext *write_ctx =
        (S3AsyncMotrKVSWriterContext *)ctx->application_context;

    op->op_rc = S3FakeMotrKvs::instance()->kv_write(
        op->op_entity->en_id, *write_ctx->get_motr_kvs_op_ctx());
  } else if (M0_IC_DEL == op->op_code) {
    struct s3_motr_context_obj *ctx =
        (struct s3_motr_context_obj *)op->op_datum;

    S3AsyncMotrKVSWriterContext *write_ctx =
        (S3AsyncMotrKVSWriterContext *)ctx->application_context;

    op->op_rc = S3FakeMotrKvs::instance()->kv_del(
        op->op_entity->en_id, *write_ctx->get_motr_kvs_op_ctx());
  }

  // Free user event
  event_free((struct event *)user_context->user_event);
  s3_motr_op_stable(op);
  free(user_data);
}

void s3_motr_dummy_op_failed(evutil_socket_t, short events, void *user_data) {
  s3_log(S3_LOG_DEBUG, "", "Entering\n");
  struct user_event_context *user_context =
      (struct user_event_context *)user_data;
  struct m0_op *op = (struct m0_op *)user_context->app_ctx;
  struct s3_motr_context_obj *ctx = (struct s3_motr_context_obj *)op->op_datum;

  // This can be mocked from GTest but system tests call this method too
  // where m0_rc can't be mocked.
  op->op_rc = -ETIMEDOUT;  // fake network failure
  ctx->is_fake_failure = 1;
  // Free user event
  event_free((struct event *)user_context->user_event);
  s3_motr_op_failed(op);
  free(user_data);
}
