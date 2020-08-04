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

package com.seagates3.controller;

import com.seagates3.exception.DataAccessException;
import com.seagates3.model.AccessKey;
import com.seagates3.model.Requestor;
import com.seagates3.model.User;
import com.seagates3.response.ServerResponse;
import com.seagates3.response.generator.FederationTokenResponseGenerator;
import com.seagates3.service.AccessKeyService;
import com.seagates3.service.UserService;
import java.util.Map;

public class FederationTokenController extends AbstractController {

    FederationTokenResponseGenerator fedTokenResponse;

    public FederationTokenController(Requestor requestor,
            Map<String, String> requestBody) {
        super(requestor, requestBody);

        fedTokenResponse = new FederationTokenResponseGenerator();
    }

    /*
     * TODO
     * Store user policy
     */
    @Override
    public ServerResponse create() throws DataAccessException {
        String userName = requestBody.get("Name");

        int duration;
        if (requestBody.containsKey("DurationSeconds")) {
            duration = Integer.parseInt(requestBody.get("DurationSeconds"));
        } else {
            duration = 43200;
        }

        User user = UserService.createFederationUser(requestor.getAccount(),
                userName);

        AccessKey accessKey = AccessKeyService.createFedAccessKey(user,
                duration);
        if (accessKey == null) {
            return fedTokenResponse.internalServerError();
        }

        return fedTokenResponse.generateCreateResponse(user, accessKey);
    }
}
