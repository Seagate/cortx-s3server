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

package com.seagates3.s3service;

import java.io.IOException;

import org.apache.http.client.ClientProtocolException;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.ExpectedException;

import com.seagates3.exception.S3RequestInitializationException;

public class S3RestClientTest {

    @Rule
    public ExpectedException thrown= ExpectedException.none();

    @BeforeClass
    public static void setUpBeforeClass() throws Exception {
    }

    @Before
    public void setUp() throws Exception {
    }

    @Test
    public void testPostRequest_NoS3Server() throws ClientProtocolException,
                            S3RequestInitializationException, IOException {
        thrown.expect(IOException.class);

        S3RestClient s3cli = new S3RestClient();
        s3cli.setCredentials("testak", "testsk");
        s3cli.setResource("/account/id1");
        //Some invalid s3 url
        s3cli.setURL("http://s3-nonexistant.com1");

        s3cli.postRequest();
    }

    @Test
    public void testDeleteRequest_NoS3Server() throws ClientProtocolException,
                               S3RequestInitializationException, IOException {

        thrown.expect(IOException.class);

        S3RestClient s3cli = new S3RestClient();
        s3cli.setCredentials("testak", "testsk");
        s3cli.setResource("/account/id1");
        //Some invalid s3 url
        s3cli.setURL("http://s3-nonexistant.com1");

        s3cli.deleteRequest();

    }

    @Test
    public void testPostInvalidRequest_NoResource() throws ClientProtocolException,
                        S3RequestInitializationException, IOException {

        thrown.expect(S3RequestInitializationException.class);
        thrown.expectMessage("Failed to initialize");

        S3RestClient s3cli = new S3RestClient();
        s3cli.setCredentials("testak", "testsk");

        s3cli.setURL("http://s3.seagate.com");

        //Not Setting resource value, throws exception
        //s3cli.setResource("/account/id1");

        s3cli.postRequest();
    }

    @Test
    public void testDeleteInvalidRequest_NoURL() throws ClientProtocolException,
                           S3RequestInitializationException, IOException {

        thrown.expect(S3RequestInitializationException.class);
        thrown.expectMessage("Failed to initialize");

        //Not setting URL it will be null and throw a exception
        S3RestClient s3cli = new S3RestClient();
        s3cli.setCredentials("testak", "testsk");
        s3cli.setResource("/account/id1");

        //Not setting URL, throws a exception
        //s3cli.setURL("http://s3.seagate.com");

        s3cli.deleteRequest();

    }

    @Test
    public void testDeleteInvalidRequest_NoCredential()
                           throws ClientProtocolException,
                           S3RequestInitializationException, IOException {

        thrown.expect(S3RequestInitializationException.class);
        thrown.expectMessage("Failed to initialize");
        //Not setting URL it will be null and throw a exception
        S3RestClient s3cli = new S3RestClient();

        s3cli.setResource("/account/id1");
        s3cli.setURL("http://s3.seagate.com");

        //Not setting credential throws a exception
        //s3cli.setCredentials("testak", "testsk");

        s3cli.deleteRequest();

    }

    @Test
    public void testDeleteInvalidRequest_NoAccessKey()
                         throws ClientProtocolException,
                         S3RequestInitializationException, IOException {

        thrown.expect(S3RequestInitializationException.class);
        thrown.expectMessage("Failed to initialize");
        //Not setting URL it will be null and throw a exception
        S3RestClient s3cli = new S3RestClient();

        s3cli.setResource("/account/id1");
        s3cli.setURL("http://s3.seagate.com");

        s3cli.setCredentials("", "testsk");

        s3cli.deleteRequest();

    }

}
