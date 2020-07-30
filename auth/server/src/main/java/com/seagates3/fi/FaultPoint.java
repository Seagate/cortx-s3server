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
