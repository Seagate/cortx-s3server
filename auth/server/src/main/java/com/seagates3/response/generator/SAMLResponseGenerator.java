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

import com.seagates3.response.ServerResponse;
import com.seagates3.response.formatter.xml.AssumeRoleWithSAMLResponseFormatter;
import io.netty.handler.codec.http.HttpResponseStatus;

public class SAMLResponseGenerator
        extends AbstractResponseGenerator {

    public ServerResponse invalidIdentityToken() {
        String errorMessage = "The web identity token that was passed could "
                + "not be validated. Get a new identity token from the "
                + "identity provider and then retry the request.";

        return new AssumeRoleWithSAMLResponseFormatter().formatErrorResponse(
                HttpResponseStatus.BAD_REQUEST, "InvalidIdentityToken",
                errorMessage);
    }

    public ServerResponse expiredToken() {
        String errorMessage = "The web identity token that was passed is "
                + "expired or is not valid. Get a new identity token from the "
                + "identity provider and then retry the request.";

        return new AssumeRoleWithSAMLResponseFormatter().formatErrorResponse(
                HttpResponseStatus.BAD_REQUEST, "ExpiredToken",
                errorMessage);
    }

    public ServerResponse idpRejectedClaim() {
        String errorMessage = "The identity provider (IdP) reported that "
                + "authentication failed. This might be because the claim is "
                + "invalid.\nIf this error is returned for the "
                + "AssumeRoleWithWebIdentity operation, it can also mean that "
                + "the claim has expired or has been explicitly revoked.";

        return new AssumeRoleWithSAMLResponseFormatter().formatErrorResponse(
                HttpResponseStatus.FORBIDDEN, "IDPRejectedClaim",
                errorMessage);
    }
}
