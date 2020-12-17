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

package com.seagates3.authserver;

public
class AuthServerConstants {
 public
  final static String INSTALL_DIR = "/opt/seagate/cortx/auth";

 public
  final static String RESOURCE_DIR = "/opt/seagate/cortx/auth/resources";

 public
  final static int MAX_AND_DEFAULT_ROOT_USER_DURATION = 3600;

 public
  final static String DEFAULT_IAM_USER_DURATION = "43200";

 public
  final static int MAX_IAM_USER_DURATION = 129600;

 public
  final static int MIN_ROOT_IAM_USER_DURATION = 900;

 public
  final static String PERMANENT_KEY_PREFIX = "AKIA";

 public
  final static String TEMPORARY_KEY_PREFIX = "ASIA";
}
