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

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

class FaultPoint {

    private final Logger LOGGER = LoggerFactory.getLogger(FaultPoint.class.getName());
    private enum Mode { FAIL_ALWAYS, FAIL_ONCE, FAIL_N_TIMES, SKIP_FIRST_N_TIMES }
    private String failLocation;
    private Mode mode;
    private int failOrSkipCount;
    private int hitCount;

    FaultPoint(String failLocation, String mode, int failOrSkipCount) {
        this.failLocation = failLocation;
        this.mode = Mode.valueOf(mode.toUpperCase());
        this.failOrSkipCount = failOrSkipCount;
    }

    private boolean checkAndUpdateState() {
        hitCount += 1;
        if (mode == Mode.FAIL_ONCE) {
            if (hitCount > 1) {
                return false;
            }
        } else if (mode == Mode.FAIL_N_TIMES) {
            if (hitCount > failOrSkipCount) {
                return false;
            }
        } else if (mode == Mode.SKIP_FIRST_N_TIMES) {
            if (hitCount <= failOrSkipCount) {
                return false;
            }
        }

        return true;
    }

    public boolean isActive() {
        if(checkAndUpdateState()) {
            LOGGER.debug("FailLocation: " + failLocation + " HitCount: " + hitCount);
            return true;
        }

        return false;
    }
}
