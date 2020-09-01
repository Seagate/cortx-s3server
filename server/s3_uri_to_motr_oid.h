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

#pragma once

#ifndef __S3_SERVER_S3_URI_TO_MOTR_OID_H__
#define __S3_SERVER_S3_URI_TO_MOTR_OID_H__

#include "s3_common.h"
#include "s3_motr_wrapper.h"

EXTERN_C_BLOCK_BEGIN
#include "motr/client.h"
EXTERN_C_BLOCK_END

int S3UriToMotrOID(std::shared_ptr<MotrAPI> s3_motr_api, const char *uri_name,
                   const std::string &request_id, m0_uint128 *ufid,
                   S3MotrEntityType type = S3MotrEntityType::object);
#endif
