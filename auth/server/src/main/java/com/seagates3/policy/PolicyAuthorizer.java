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

package com.seagates3.policy;

import java.util.Map;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.seagates3.model.Requestor;
import com.seagates3.response.ServerResponse;

public
abstract class PolicyAuthorizer extends PolicyAuthorizerConstants {

 protected
  final Logger LOGGER =
      LoggerFactory.getLogger(PolicyAuthorizer.class.getName());

 public
  abstract ServerResponse
      authorizePolicy(Requestor requestor, Map<String, String> requestBody);
}
