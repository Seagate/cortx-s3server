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

import com.google.gson.Gson;
import com.google.gson.GsonBuilder;

public class JSONUtil {

    /**
     * Serialize the class into a JSON String.
     *
     * @param obj
     * @return
     */
    public static String serializeToJson(Object obj) {
        Gson gson = new GsonBuilder().create();
        return gson.toJson(obj);
    }

    /**
     * Convert json to SAML Metadata Tokens object.
     *
     * @param jsonBody
     * @param deserializeClass
     * @return
     */
    public static Object deserializeFromJson(String jsonBody,
            Class<?> deserializeClass) {
        Gson gson = new Gson();

        return gson.fromJson(jsonBody, deserializeClass);
    }

}
