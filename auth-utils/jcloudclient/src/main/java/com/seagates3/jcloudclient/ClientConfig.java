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

package com.seagates3.jcloudclient;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;
import java.util.Properties;


public class ClientConfig {

    public static String getEndPoint(ClientService service) throws IOException {
        Properties endpointConfig = new Properties();
        String endpoint = null;

        InputStream input = new FileInputStream(JCloudClient.CONFIG_FILE_NAME);
        endpointConfig.load(input);

        if (service.equals(service.S3)) {
            if (Boolean.valueOf(endpointConfig.getProperty("use_https")))
                endpoint = "https://" + endpointConfig.getProperty("s3_endpoint")
                              + ":" + endpointConfig.getProperty("s3_https_port");
            else
                endpoint = "http://" + endpointConfig.getProperty("s3_endpoint")
                              + ":" + endpointConfig.getProperty("s3_http_port");

        } else {
            if (Boolean.valueOf(endpointConfig.getProperty("use_https")))
                endpoint = "https://" + endpointConfig.getProperty("iam_endpoint")
                              +":"+ endpointConfig.getProperty("iam_https_port");
            else
                endpoint = "http://" + endpointConfig.getProperty("iam_endpoint")
                              +":"+ endpointConfig.getProperty("iam_http_port");

        }
        return endpoint;
    }
}
