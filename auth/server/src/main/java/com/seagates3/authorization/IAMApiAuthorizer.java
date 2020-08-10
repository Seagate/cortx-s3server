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

package com.seagates3.authorization;

import com.seagates3.response.ServerResponse;
import java.util.Map;
import com.seagates3.exception.InvalidUserException;
import com.seagates3.model.Requestor;
import com.seagates3.response.generator.ResponseGenerator;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public
class IAMApiAuthorizer {

 private
  static final Logger LOGGER =
      LoggerFactory.getLogger(IAMApiAuthorizer.class.getName());

 private
  ResponseGenerator responseGenerator = new ResponseGenerator();

  /**
   * Authorize user to perform invoked action with given
   * access key and secret key.
   */
 public
  void authorize(Requestor requestor,
                 Map<String, String> requestBody) throws InvalidUserException {
    if (!(validateIfUserCanPerformAction(requestor, requestBody))) {
      LOGGER.debug("User doesn't have permission to perform invoked action");
      ServerResponse serverResponse = responseGenerator.invalidUser();
      throw new InvalidUserException(serverResponse);
    }
  }

  /**
   * Validate user with credentials given and action.
   *
   * Check if user with credentials given and action requested is allowed to
   * perform action.
   *
   */
 public
  Boolean validateIfUserCanPerformAction(Requestor requestor,
                                         Map<String, String> requestBody) {
    if (hasRootUserCredentials(requestor)) {
      return true;
    }

    if (isSameUser(requestBody, requestor)) {
      return true;
    }
    return false;
  }

  /**
   * Check if User has provided root credentials
   */
 private
  static Boolean hasRootUserCredentials(Requestor requestor) {
    return (requestor.getName().equals("root"));
  }

  /**
   * Check if User is performing operation on self only
   */
 private
  static Boolean isSameUser(Map<String, String> requestBody,
                            Requestor requestor) {
    return (requestBody.get("UserName").equals(requestor.getName()));
  }

  /**
   * Check if Account is performing operation on self only
   */
 private
  static Boolean isSameAccount(Map<String, String> requestBody,
                               Requestor requestor) {
    return requestBody.get("AccountName")
        .equals(requestor.getAccount().getName());
  }

  /**
    * Authorize user at root level to perform invoked action with given
    * access key and secret key.
    */
 public
  void authorizeRootUser(Requestor requestor, Map<String, String> requestBody)
      throws InvalidUserException {
    if (!(hasRootUserCredentials(requestor))) {
      LOGGER.debug("User doesn't have permission to perform invoked action");
      ServerResponse serverResponse = responseGenerator.invalidUser();
      throw new InvalidUserException(serverResponse);
    }
    // If It's Account action like createaccountloginprofile
    // then check if its same account
    if ((requestBody.get("AccountName") != null) &&
        (!isSameAccount(requestBody, requestor))) {
      LOGGER.debug("User doesn't have permission to perform invoked action");
      ServerResponse serverResponse = responseGenerator.invalidUser();
      throw new InvalidUserException(serverResponse);
    }
  }
}
