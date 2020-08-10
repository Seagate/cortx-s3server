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

package com.seagates3.authentication;

import java.util.Collections;
import java.util.HashMap;
import java.util.Map;

import com.seagates3.exception.InvalidTokenException;
import com.seagates3.model.Requestor;

public interface AWSSign {
    /**
     * Map the AWS signing algorithm against the corresponding hashing function.
     */
    public static final Map<String, String> AWSHashFunction =
            Collections.unmodifiableMap(
                    new HashMap<String, String>() {
                        {
                            put("AWS4-HMAC-SHA256", "hashSHA256");
                        }
                    });

    /*
     * Authenticate the request using AWS algorithm.
     */
    public Boolean authenticate(ClientRequestToken clientRequestToken,
                       Requestor requestor) throws InvalidTokenException;
}
