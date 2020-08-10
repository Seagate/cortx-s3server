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

package com.seagates3.controller;

import com.seagates3.exception.FaultPointException;
import com.seagates3.fi.FaultPoints;
import com.seagates3.response.ServerResponse;
import io.netty.handler.codec.http.HttpResponseStatus;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.powermock.api.mockito.mockpolicies.Slf4jMockPolicy;
import org.powermock.core.classloader.annotations.MockPolicy;
import org.powermock.core.classloader.annotations.PowerMockIgnore;
import org.powermock.core.classloader.annotations.PrepareForTest;
import org.powermock.modules.junit4.PowerMockRunner;

import java.util.Map;
import java.util.TreeMap;

import static org.hamcrest.CoreMatchers.containsString;
import static org.junit.Assert.*;
import static org.powermock.api.mockito.PowerMockito.doThrow;
import static org.powermock.api.mockito.PowerMockito.mock;
import static org.powermock.api.mockito.PowerMockito.mockStatic;
import static org.powermock.api.mockito.PowerMockito.when;

@RunWith(PowerMockRunner.class)
@MockPolicy(Slf4jMockPolicy.class)
@PowerMockIgnore({"javax.management.*"})
@PrepareForTest({FaultPointsController.class, FaultPoints.class})
public class FaultPointsControllerTest {

    private FaultPoints faultPoints;
    private Map<String, String> requestBody;
    private FaultPointsController controller;
    private ServerResponse serverResponse;

    @Before
    public void setUp() throws Exception {
        mockStatic(FaultPoints.class);

        faultPoints = mock(FaultPoints.class);
        serverResponse = mock(ServerResponse.class);

        when(FaultPoints.getInstance()).thenReturn(faultPoints);

        requestBody = new TreeMap<>();
        controller = new FaultPointsController();
    }

    @Test
    public void setTest_TooFewParameters() throws Exception {
        ServerResponse result = controller.set(requestBody);

        assertThat(result.getResponseBody(), containsString("Too few parameters."));
        assertEquals(HttpResponseStatus.BAD_REQUEST, result.getResponseStatus());
    }

    @Test
    public void setTest_SetFaultPoint() throws Exception {
        requestBody.put("FaultPoint", "LDAP_SEARCH");
        requestBody.put("Mode", "FAIL_ONCE");
        requestBody.put("Value", "1");

        ServerResponse result = controller.set(requestBody);

        assertEquals(HttpResponseStatus.CREATED, result.getResponseStatus());
        assertEquals("Fault point set successfully.", result.getResponseBody());
    }

    @Test
    public void setTest_IllegalArgumentException() throws Exception {
        requestBody.put("FaultPoint", "LDAP_SEARCH");
        requestBody.put("Mode", "FAIL_ONCE");
        requestBody.put("Value", "1");
        doThrow(new IllegalArgumentException()).when(faultPoints)
                .setFaultPoint("LDAP_SEARCH", "FAIL_ONCE", 1);

        ServerResponse result = controller.set(requestBody);

        assertThat(result.getResponseBody(), containsString(
                "An invalid or out-of-range value was supplied for the input parameter."));
        assertEquals(HttpResponseStatus.BAD_REQUEST, result.getResponseStatus());
    }

    @Test
    public void setTest_FaultPointException() throws Exception {
        requestBody.put("FaultPoint", "LDAP_SEARCH");
        requestBody.put("Mode", "FAIL_ONCE");
        requestBody.put("Value", "1");
        doThrow(new FaultPointException("FI Exception")).when(faultPoints)
                .setFaultPoint("LDAP_SEARCH", "FAIL_ONCE", 1);

        ServerResponse result = controller.set(requestBody);

        assertThat(result.getResponseBody(), containsString(
                "<Code>FaultPointAlreadySet</Code><Message>FI Exception</Message>"));
        assertEquals(HttpResponseStatus.CONFLICT, result.getResponseStatus());
    }

    @Test
    public void resetTest_TooFewParameters() throws Exception {
        ServerResponse result = controller.reset(requestBody);

        assertThat(result.getResponseBody(), containsString("Invalid parameters."));
        assertEquals(HttpResponseStatus.BAD_REQUEST, result.getResponseStatus());
    }

    @Test
    public void resetTest_ResetFaultPoint() throws Exception {
        requestBody.put("FaultPoint", "LDAP_SEARCH");

        ServerResponse result = controller.reset(requestBody);

        assertEquals(HttpResponseStatus.OK, result.getResponseStatus());
        assertEquals("Fault point deleted successfully.", result.getResponseBody());
    }

    @Test
    public void resetTest_IllegalArgumentException() throws Exception {
        requestBody.put("FaultPoint", "");
        doThrow(new IllegalArgumentException("Fault point can't be empty or null"))
                .when(faultPoints).resetFaultPoint("");

        ServerResponse result = controller.reset(requestBody);

        assertThat(result.getResponseBody(),
                containsString("Fault point can't be empty or null"));
        assertEquals(HttpResponseStatus.BAD_REQUEST, result.getResponseStatus());
    }

    @Test
    public void resetTest_FaultPointException() throws Exception {
        requestBody.put("FaultPoint", "LDAP_SEARCH");
        doThrow(new FaultPointException("Fault point LDAP_SEARCH is not set"))
                .when(faultPoints).resetFaultPoint("LDAP_SEARCH");

        ServerResponse result = controller.reset(requestBody);

        assertThat(result.getResponseBody(), containsString("<Code>FaultPointNotSet</Code>" +
                "<Message>Fault point LDAP_SEARCH is not set</Message>"));
        assertEquals(HttpResponseStatus.CONFLICT, result.getResponseStatus());
    }
}