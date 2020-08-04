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

package com.seagates3.util;

public class ARNUtil {

    private static final String PARTITION = "seagate";
    private static final String SERVICE = "iam";

    public static String createARN(String accountId, String resource) {
        return String.format("arn:%s:%s::%s:%s", PARTITION, SERVICE,
                accountId, resource);
    }

    public static String createARN(String accountId, String resourceType,
            String resource) {
        return String.format("arn:%s:%s::%s:%s/%s", PARTITION, SERVICE,
                accountId, resourceType, resource);
    }
}
