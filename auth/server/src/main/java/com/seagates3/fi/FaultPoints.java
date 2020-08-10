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

package com.seagates3.fi;

import com.seagates3.exception.FaultPointException;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.HashMap;
import java.util.Map;

/*
 * This class is not thread safe.
 * Instance is created during startup phase in main method.
 */
public class FaultPoints {

    private final Logger LOGGER = LoggerFactory.getLogger(FaultPoints.class.getName());
    private Map<String, FaultPoint> faults = new HashMap<>();
    private static FaultPoints instance;

    private FaultPoints() {}

    public static void init() {
        instance = new FaultPoints();
    }

    public static FaultPoints getInstance() {
        return instance;
    }

    public static boolean fiEnabled() {
        if (instance == null) {
            return false;
        }

        return true;
    }

    public boolean isFaultPointSet(String failLocation) {
        return faults.containsKey(failLocation);
    }

    public boolean isFaultPointActive(String failLocation) {
        if (isFaultPointSet(failLocation)) {
            return faults.get(failLocation).isActive();
        }

        return false;
    }

    public void setFaultPoint(String failLocation, String mode, int value)
            throws FaultPointException {
        if (failLocation == null || failLocation.isEmpty()) {
            LOGGER.debug("Fault point can't be empty or null");
            throw new IllegalArgumentException("Invalid fault point");
        }
        if (faults.containsKey(failLocation)) {
            LOGGER.debug("Fault point " + failLocation + " already set");
            throw new FaultPointException("Fault point " + failLocation + " already set");
        }

        FaultPoint faultPoint = new FaultPoint(failLocation, mode, value);
        faults.put(failLocation, faultPoint);
        LOGGER.debug("Fault point: " + failLocation + " is set successfully.");
    }

    public void resetFaultPoint(String failLocation) throws FaultPointException {
        if (failLocation == null || failLocation.isEmpty()) {
            LOGGER.debug("Fault point can't be empty or null");
            throw new IllegalArgumentException("Invalid fault point");
        }
        if (!faults.containsKey(failLocation)) {
            LOGGER.debug("Fault point " + failLocation + " is not set");
            throw new FaultPointException("Fault point " + failLocation + " is not set");
        }

        faults.remove(failLocation);
        LOGGER.debug("Fault point: " + failLocation + " is removed successfully");
    }
}
