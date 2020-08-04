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

import java.io.IOException;
import java.util.List;
import java.util.Map;
import java.util.TreeMap;

import io.netty.handler.codec.http.FullHttpRequest;
import io.netty.handler.codec.http.multipart.DefaultHttpDataFactory;
import io.netty.handler.codec.http.multipart.HttpPostRequestDecoder;
import io.netty.handler.codec.http.multipart.InterfaceHttpData;
import io.netty.handler.codec.http.multipart.Attribute;
import io.netty.handler.codec.http.multipart.InterfaceHttpData.HttpDataType;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class AuthRequestDecoder extends HttpPostRequestDecoder {

    private final Logger LOGGER = LoggerFactory.getLogger(
            AuthServerHandler.class.getName());

    public AuthRequestDecoder(FullHttpRequest fullHttpRequest) {
        super(new DefaultHttpDataFactory(DefaultHttpDataFactory.MINSIZE),
                fullHttpRequest);
    }

    public Map<String, String> getRequestBodyAsMap() {
        return parseRequestBody(getBodyHttpDatas());
    }

    private Map<String, String> parseRequestBody(List<InterfaceHttpData> datas) {
        Map<String, String> requestBody;
        requestBody = new TreeMap<>(String.CASE_INSENSITIVE_ORDER);
        String key;
        String value;

        LOGGER.debug("Request body attributes:");
        for (InterfaceHttpData data : datas) {
            if (data.getHttpDataType() == HttpDataType.Attribute) {
                Attribute attribute = (Attribute) data;
                key = attribute.getName();
                try {
                    value = attribute.getValue();
                    if (requestBody.containsKey(key) && key.toLowerCase().startsWith("x-amz-")) {
                        value = requestBody.get(key) + "," + value;
                    }
                    requestBody.put(key, value);
                    LOGGER.debug("{}: {}", key, value);
                } catch (IOException e) {
                    LOGGER.warn("Failed to get value of {} attribute. Error: {}",
                            key, e.getMessage());
                }
            }
        }

        return requestBody;
    }
}