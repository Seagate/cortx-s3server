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

import com.google.gson.JsonSyntaxException;
import com.seagates3.model.SAMLMetadataTokens;
import org.junit.Test;

import static org.junit.Assert.*;

public class JSONUtilTest {

    @Test
    public void serializeToJsonTest() {
        assertNotNull(JSONUtil.serializeToJson(new Object()));
    }

    @Test
    public void deserializeFromJsonTest() {
        String jsonBody = "{username: seagate}";

        assertNotNull(JSONUtil.deserializeFromJson(jsonBody, SAMLMetadataTokens.class));
    }

    @Test
    public void deserializeFromJsonTest_ShouldReturnNullIfJsonBodyIsEmpty() {
        String jsonBody = "";
        assertNull(JSONUtil.deserializeFromJson(jsonBody, SAMLMetadataTokens.class));
    }

    @Test(expected = JsonSyntaxException.class)
    public void deserializeFromJsonTest_InvalidSyntaxException() {
        JSONUtil.deserializeFromJson("InvalidJSONBody", SAMLMetadataTokens.class);
    }
}
