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
