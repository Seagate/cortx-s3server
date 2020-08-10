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
import com.seagates3.response.generator.FaultPointsResponseGenerator;

import java.util.Map;

public class FaultPointsController {

    private final FaultPoints faultPoints = FaultPoints.getInstance();
    public FaultPointsResponseGenerator responseGenerator = new FaultPointsResponseGenerator();

    public ServerResponse set(Map<String, String> requestBody) {
        if (!requiredParamsExist(requestBody)) {
            return responseGenerator.badRequest("Too few parameters.");
        }

        try {
            String faultPoint = requestBody.get("FaultPoint");
            String mode = requestBody.get("Mode");
            int value = Integer.parseInt(requestBody.get("Value"));
            faultPoints.setFaultPoint(faultPoint, mode, value);
        } catch (IllegalArgumentException e) {
            return responseGenerator.invalidParametervalue();
        } catch (FaultPointException e) {
            return responseGenerator.faultPointAlreadySet(e.getMessage());
        } catch (Exception e) {
            return responseGenerator.internalServerError();
        }

        return responseGenerator.setSuccessful();
    }

    public ServerResponse reset(Map<String, String> requestBody) {
        if (!requestBody.containsKey("FaultPoint")) {
            return responseGenerator.badRequest("Invalid parameters.");
        }

        String faultPoint = requestBody.get("FaultPoint");
        try {
            faultPoints.resetFaultPoint(faultPoint);
        } catch (IllegalArgumentException e) {
            return responseGenerator.invalidParametervalue(e.getMessage());
        } catch (FaultPointException e) {
            return responseGenerator.faultPointNotSet(e.getMessage());
        } catch (Exception e) {
            return responseGenerator.internalServerError();
        }

        return responseGenerator.resetSuccessful();
    }

    private boolean requiredParamsExist(Map<String, String> requestBody) {
        if (requestBody.containsKey("FaultPoint") &&
                requestBody.containsKey("Mode") &&
                requestBody.containsKey("Value")) {
            return true;
        }

        return false;
    }
}
