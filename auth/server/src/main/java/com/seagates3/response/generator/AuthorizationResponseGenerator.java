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

package com.seagates3.response.generator;

import com.seagates3.authserver.AuthServerConfig;
import com.seagates3.model.Requestor;
import com.seagates3.response.ServerResponse;
import com.seagates3.response.formatter.xml.AuthorizationResponseFormatter;
import java.util.LinkedHashMap;

public class AuthorizationResponseGenerator extends AbstractResponseGenerator {

  public
   ServerResponse generateAuthorizationResponse(Requestor requestor,
                                                String acpXml) {
        LinkedHashMap responseElements = new LinkedHashMap();
        if (requestor != null) {
          responseElements.put("UserId", requestor.getId());
          responseElements.put("UserName", requestor.getName());
          responseElements.put("AccountId", requestor.getAccount().getId());
          responseElements.put("AccountName", requestor.getAccount().getName());
          responseElements.put("CanonicalId",
                               requestor.getAccount().getCanonicalId());
        } else {
          responseElements.put("AllUserRequest", "true");
        }

        return (ServerResponse) new AuthorizationResponseFormatter().authorized(
            responseElements, AuthServerConfig.getReqId(), acpXml);
    }
}
