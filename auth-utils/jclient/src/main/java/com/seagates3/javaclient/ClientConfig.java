package com.seagates3.javaclient;

import com.amazonaws.ClientConfiguration;
import com.amazonaws.Protocol;
import com.amazonaws.auth.AWSCredentials;
import com.amazonaws.auth.BasicAWSCredentials;
import com.amazonaws.auth.BasicSessionCredentials;
import com.amazonaws.services.s3.AmazonS3Client;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;
import java.util.Properties;

public class ClientConfig {


    public static ClientConfiguration getClientConfiguration() {
        ClientConfiguration config = new ClientConfiguration();
        config.setProtocol(Protocol.HTTP);

        return config;
    }

    public static String getEndPoint(Class<?> service) throws IOException {
        Properties endpointConfig = new Properties();
        String endpoint = null;

        InputStream input = new FileInputStream(JavaClient.CONFIG_FILE_NAME);
        endpointConfig.load(input);

        if (service.equals(AmazonS3Client.class)) {
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

    public static AWSCredentials getCreds(String accessKeyId, String secretkey) {
        return new BasicAWSCredentials(accessKeyId, secretkey);
    }

    public static AWSCredentials getCreds(String accessKeyId, String secretkey,
            String sessionToken) {
        return new BasicSessionCredentials(accessKeyId, secretkey, sessionToken);
    }
}
