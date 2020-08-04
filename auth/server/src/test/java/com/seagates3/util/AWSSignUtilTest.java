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

import static org.junit.Assert.*;

import org.junit.Test;

public class AWSSignUtilTest {

    @Test
    public void testCalculateSignatureAWSV2() {

        String d = "Tue, 10 Jul 2018 06:38:28 GMT";
        String payload = "POST" + "\n\n\n" + d + "\n" + "/account/actid1";
        String secretKey = "kjldjsoijweoiiwjiwfjjijwpeijfpfjpfjep+kdo=";

        String expectedSignedValue = "cKqcWGZFIFqFYufbCeBXVhpXXm0=";

        String signedValue = AWSSignUtil.calculateSignatureAWSV2(payload, secretKey);

        assertTrue(signedValue.equals(expectedSignedValue));

    }

}
