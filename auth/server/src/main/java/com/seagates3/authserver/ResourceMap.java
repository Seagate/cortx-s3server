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

package com.seagates3.authserver;

import org.apache.commons.lang.StringUtils;

public class ResourceMap {

    private final String controllerName;
    private final String action;

    private final String VALIDATOR_PACKAGE = "com.seagates3.parameter.validator";
    private final String CONTROLLER_PACKAGE = "com.seagates3.controller";

    public ResourceMap(String controllerName, String action) {
        this.controllerName = controllerName;
        this.action = action;
    }

    /**
     * Return the full class path of the controller.
     *
     * @return
     */
    public String getControllerClass() {
        return String.format("%s.%sController", CONTROLLER_PACKAGE,
                controllerName);
    }

    /**
     * Return the full name of the validator class.
     *
     * @return
     */
    public String getParamValidatorClass() {
        return String.format("%s.%sParameterValidator", VALIDATOR_PACKAGE,
                controllerName);
    }

    /**
     * Return the Controller Action to be invoked.
     *
     * @return Action.
     */
    public String getControllerAction() {
        return action;
    }

    /**
     * Return the Controller Action to be invoked.
     *
     * @return Action.
     */
    public String getParamValidatorMethod() {
        return String.format("isValid%sParams", StringUtils.capitalize(action));
    }

}
