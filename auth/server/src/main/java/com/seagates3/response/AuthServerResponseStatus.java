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

package com.seagates3.response;

public enum AuthServerResponseStatus {
    OK(""),
    NO_SUCH_ENTITY("noSuchEntity"),
    INACTIVE_ACCESS_KEY("inactiveAccessKey"),
    INVALID_CLIENT_TOKEN_ID("invalidClientToken"),
    EXPIRED_CREDENTIAL("expiredCredential"),
    INCORRECT_SIGNATURE("incorrectSignature");

    private final String method;

    /**
     * @param text
     */
    private AuthServerResponseStatus(final String text) {
        this.method = text;
    }

    /* (non-Javadoc)
     * @see java.lang.Enum#toString()
     */
    @Override
    public String toString() {
        return method;
    }
}
