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

public
class ArnParser {

  // arn:aws:(s3|iam):[region]:[accountID]:<resource>
 protected
  String regEx = "arn:aws:(s3|iam):[A-Za-z0-9-]*:" +
                 "[a-zA-Z0-9~@#$^*\\\\/_.-]*:[a-zA-Z0-9~@#$^*\\\\/_.-]+";

  /**
   * validates ARN format for generic S3/IAM - services
   * arn:aws:(s3|iam):[region]:[accountID]:<resource>
   * @param arn
   * @return
   */
 public
  boolean isArnFormatValid(String arn) {
    boolean isArnValid = false;
    if (arn != null && arn.matches(regEx)) {
      isArnValid = true;
    }
    return isArnValid;
  }
}
