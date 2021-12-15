/*
 * Copyright (c) 2021 Seagate Technology LLC and/or its Affiliates
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

#include "s3_error_codes.h"
#include "s3_log.h"
#include "s3_api_handler.h"
#include "s3_data_usage_action.h"
#include <iostream>

S3DataUsageAction::S3DataUsageAction(std::shared_ptr<S3RequestObject> req)
    : S3Action(req, true, nullptr, true, true), schema("hii") {

  setup_steps();
}
S3DataUsageAction::~S3DataUsageAction() {
  s3_log(S3_LOG_DEBUG, "", "%s\n", __func__);
}
void S3DataUsageAction::setup_steps() { send_response_to_s3_client(); }

void S3DataUsageAction::send_response_to_s3_client() {
  s3_log(S3_LOG_DEBUG, "", "%s Entry \n", __func__);
  request->send_response(S3HttpSuccess200, schema);
}
