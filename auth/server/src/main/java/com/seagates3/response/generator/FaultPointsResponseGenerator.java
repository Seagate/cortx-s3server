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

package com.seagates3.response.generator;

import com.seagates3.response.ServerResponse;
import io.netty.handler.codec.http.HttpResponseStatus;

public class FaultPointsResponseGenerator extends AbstractResponseGenerator {

    public ServerResponse badRequest(String errorMessage) {
        return formatResponse(HttpResponseStatus.BAD_REQUEST,
                "BadRequest", errorMessage);
    }

    public ServerResponse invalidParametervalue(String errorMessage) {
        return formatResponse(HttpResponseStatus.BAD_REQUEST,
                "InvalidParameterValue", errorMessage);
    }

    public ServerResponse faultPointAlreadySet(String errorMessage) {

        return formatResponse(HttpResponseStatus.CONFLICT,
                "FaultPointAlreadySet", errorMessage);
    }

    public ServerResponse faultPointNotSet(String errorMessage) {

        return formatResponse(HttpResponseStatus.CONFLICT,
                "FaultPointNotSet", errorMessage);
    }

    public ServerResponse setSuccessful() {
        return new ServerResponse(HttpResponseStatus.CREATED,
                "Fault point set successfully.");
    }

    public ServerResponse resetSuccessful() {
        return new ServerResponse(HttpResponseStatus.OK,
                "Fault point deleted successfully.");
    }
}
