/*
 * COPYRIGHT 2016 SEAGATE LLC
 *
 * THIS DRAWING/DOCUMENT, ITS SPECIFICATIONS, AND THE DATA CONTAINED
 * HEREIN, ARE THE EXCLUSIVE PROPERTY OF SEAGATE TECHNOLOGY
 * LIMITED, ISSUED IN STRICT CONFIDENCE AND SHALL NOT, WITHOUT
 * THE PRIOR WRITTEN PERMISSION OF SEAGATE TECHNOLOGY LIMITED,
 * BE REPRODUCED, COPIED, OR DISCLOSED TO A THIRD PARTY, OR
 * USED FOR ANY PURPOSE WHATSOEVER, OR STORED IN A RETRIEVAL SYSTEM
 * EXCEPT AS ALLOWED BY THE TERMS OF SEAGATE LICENSES AND AGREEMENTS.
 *
 * YOU SHOULD HAVE RECEIVED A COPY OF SEAGATE'S LICENSE ALONG WITH
 * THIS RELEASE. IF NOT PLEASE CONTACT A SEAGATE REPRESENTATIVE
 * http://www.seagate.com/contact
 *
 * Original author: Sushant Mane <sushant.mane@seagate.com>
 * Original creation date: 07-Jul-2016
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

        LOGGER.debug("Request Body:");
        for (InterfaceHttpData data : datas) {
            if (data.getHttpDataType() == HttpDataType.Attribute) {
                Attribute attribute = (Attribute) data;
                try {
                    key = attribute.getName();
                    value = attribute.getValue();
                    if (requestBody.containsKey(key) && key.toLowerCase().startsWith("x-amz-")) {
                        value = requestBody.get(key) + "," + value;
                    }
                    requestBody.put(key, value);
                    LOGGER.debug(attribute.getHttpDataType().name() + " "
                            + key + ": " + value);
                } catch (IOException readError) {
                    readError.printStackTrace();
                    LOGGER.error(attribute.getHttpDataType().name() + " "
                            + attribute.getName()
                            + " Error while reading value: "
                            + readError.getMessage());
                }
            }
        }
        return requestBody;
    }
}