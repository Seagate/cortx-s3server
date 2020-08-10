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
import com.seagates3.model.AccessKey;
import com.seagates3.model.Account;
import com.seagates3.model.User;
import com.seagates3.response.ServerResponse;
import com.seagates3.response.formatter.xml.XMLResponseFormatter;
import io.netty.handler.codec.http.HttpResponseStatus;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;

public class AccountResponseGenerator extends AbstractResponseGenerator {

    public ServerResponse generateCreateResponse(Account account, User root,
            AccessKey rootAccessKey) {
        LinkedHashMap responseElements = new LinkedHashMap();
        responseElements.put("AccountId", account.getId());
        responseElements.put("CanonicalId", account.getCanonicalId());
        responseElements.put("AccountName", account.getName());
        responseElements.put("RootUserName", root.getName());
        responseElements.put("AccessKeyId", rootAccessKey.getId());
        responseElements.put("RootSecretKeyId", rootAccessKey.getSecretKey());
        responseElements.put("Status", rootAccessKey.getStatus());

        return (ServerResponse) new XMLResponseFormatter().formatCreateResponse(
            "CreateAccount", "Account", responseElements,
            AuthServerConfig.getReqId());
    }

   public
    ServerResponse generateListResponse(Object[] responseObjects,
                                        String showAll) {
        Account[] accounts = (Account[]) responseObjects;
        ArrayList<LinkedHashMap<String, String>> accountMembers = new ArrayList<>();
        LinkedHashMap responseElements;
        List<String> configuredInternalAccounts = new ArrayList<>();
        if (!("True".equalsIgnoreCase(showAll))) {
          configuredInternalAccounts = AuthServerConfig.getS3InternalAccounts();
        }
        for (Account account : accounts) {
          if (!configuredInternalAccounts.contains(account.getName())) {
            responseElements = new LinkedHashMap();
            responseElements.put("AccountName", account.getName());
            responseElements.put("AccountId", account.getId());
            responseElements.put("CanonicalId", account.getCanonicalId());
            responseElements.put("Email", account.getEmail());
            accountMembers.add(responseElements);
        }
        }
        return new XMLResponseFormatter().formatListResponse(
            "ListAccounts", "Accounts", accountMembers, false,
            AuthServerConfig.getReqId());
    }

    public ServerResponse generateDeleteResponse() {
        return new XMLResponseFormatter().formatDeleteResponse("DeleteAccount");
    }

    public ServerResponse generateResetAccountAccessKeyResponse(Account account, User root,
            AccessKey rootAccessKey) {
        LinkedHashMap responseElements = new LinkedHashMap();
        responseElements.put("AccountId", account.getId());
        responseElements.put("CanonicalId", account.getCanonicalId());
        responseElements.put("AccountName", account.getName());
        responseElements.put("RootUserName", root.getName());
        responseElements.put("AccessKeyId", rootAccessKey.getId());
        responseElements.put("RootSecretKeyId", rootAccessKey.getSecretKey());
        responseElements.put("Status", rootAccessKey.getStatus());

        return (ServerResponse) new XMLResponseFormatter()
            .formatResetAccountAccessKeyResponse("ResetAccountAccessKey",
                                                 "Account", responseElements,
                                                 AuthServerConfig.getReqId());
    }

    @Override
    public ServerResponse entityAlreadyExists() {
        String errorMessage = "The request was rejected because it attempted "
                + "to create an account that already exists.";

        return formatResponse(HttpResponseStatus.CONFLICT,
                "EntityAlreadyExists", errorMessage);
    }

   public
    ServerResponse emailAlreadyExists() {
      String errorMessage = "The request was rejected because " +
                            "account with this email already exists.";

      return formatResponse(HttpResponseStatus.CONFLICT, "EmailAlreadyExists",
                            errorMessage);
    }
}
