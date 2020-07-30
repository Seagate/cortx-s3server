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
