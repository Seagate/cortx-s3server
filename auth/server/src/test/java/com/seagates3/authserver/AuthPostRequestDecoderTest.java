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

package com.seagates3.authserver;

import io.netty.buffer.ByteBuf;
import io.netty.buffer.ByteBufUtil;
import io.netty.buffer.Unpooled;
import io.netty.handler.codec.http.DefaultFullHttpRequest;
import io.netty.handler.codec.http.FullHttpRequest;
import io.netty.handler.codec.http.HttpMethod;
import io.netty.handler.codec.http.HttpVersion;
import org.junit.Assert;
import org.junit.Test;

import java.util.Map;
import java.util.TreeMap;

public class AuthPostRequestDecoderTest {

    @Test
    public void getRequestBodyAsMap_VerifyParameterParsing() {
        FullHttpRequest fullHttpRequest = new DefaultFullHttpRequest(
                HttpVersion.HTTP_1_1, HttpMethod.POST, "/", getRequestBodyAsByteBuf());
        AuthRequestDecoder authRequestDecoder = new AuthRequestDecoder(fullHttpRequest);
        Map<String, String> actualRequestBody = authRequestDecoder.getRequestBodyAsMap();

        Assert.assertEquals(getExpectedBody(), actualRequestBody);
    }

    private ByteBuf getRequestBodyAsByteBuf() {
        String params = "x-amz-meta-ics.simpleencryption.keyinfo=" +
                "Wrapped%3AAES%3A256%3Ax%2B7OEfeP0NgG1yG96%2FiD9F5hsyU060FWyjSqo1LjzBY%3D&" +
                "x-amz-meta-ics.simpleencryption.status=enabled&" +
                "x-amz-meta-ics.etagintegrity=S3%3Anone&" +
                "x-amz-meta-ics.simpleencryption.plaintextcontentlength=568&" +
                "x-amz-meta-ics.simpleencryption.numwrpkeys=1&" +
                "x-amz-meta-ics.simpleencryption.key0=k000%3Alkm.keybackend0&" +
                "x-amz-meta-ics.simpleencryption.wrpparams=" +
                "AES%3A256%3ACBC%3ACtpgYEqfK5Z12zpEoVxl4w%3D%3D&" +
                "x-amz-meta-ics.simpleencryption.cmbparams=xor%3AHmacSHA256&" +
                "x-amz-meta-ics.stack-description=ICStore.1%28SimpleEncryption." +
                "1%28EtagIntegrity.1%28BlobStoreConnection.1%29%29%29&" +
                "x-amz-meta-ics.meta-digest=71e7bb5fdf6784e6f9b889b2d0d39818&" +
                "x-amz-meta-ics.etagintegrity.status=enabled&" +
                "x-amz-meta-ics.meta-version=1&" +
                "x-amz-meta-ics.meta-version=2&";
        ByteBuf byteBuf = Unpooled.buffer(params.length());
        ByteBufUtil.writeUtf8(byteBuf, params);

        return byteBuf;
    }

    private Map<String, String> getExpectedBody() {
        Map<String, String> expectedRequestBody = new TreeMap<>(String.CASE_INSENSITIVE_ORDER);
        expectedRequestBody.put("x-amz-meta-ics.simpleencryption.keyinfo",
                "Wrapped:AES:256:x+7OEfeP0NgG1yG96/iD9F5hsyU060FWyjSqo1LjzBY=");
        expectedRequestBody.put("x-amz-meta-ics.simpleencryption.status", "enabled");
        expectedRequestBody.put("x-amz-meta-ics.etagintegrity", "S3:none");
        expectedRequestBody.put("x-amz-meta-ics.simpleencryption.plaintextcontentlength", "568");
        expectedRequestBody.put("x-amz-meta-ics.simpleencryption.numwrpkeys", "1");
        expectedRequestBody.put("x-amz-meta-ics.simpleencryption.key0", "k000:lkm.keybackend0");
        expectedRequestBody.put("x-amz-meta-ics.simpleencryption.wrpparams",
                "AES:256:CBC:CtpgYEqfK5Z12zpEoVxl4w==");
        expectedRequestBody.put("x-amz-meta-ics.simpleencryption.cmbparams", "xor:HmacSHA256");
        expectedRequestBody.put("x-amz-meta-ics.stack-description",
                "ICStore.1(SimpleEncryption.1(EtagIntegrity.1(BlobStoreConnection.1)))");
        expectedRequestBody.put("x-amz-meta-ics.meta-digest", "71e7bb5fdf6784e6f9b889b2d0d39818");
        expectedRequestBody.put("x-amz-meta-ics.etagintegrity.status", "enabled");
        expectedRequestBody.put("x-amz-meta-ics.meta-version", "1,2");


        return expectedRequestBody;
    }
}