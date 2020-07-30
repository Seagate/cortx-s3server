package com.seagates3.parameter.validator;

import java.util.Map;

/**
 * Validate the input for group APIs.
 */
public class GroupParameterValidator extends AbstractParameterValidator {

    /**
     * Validate the input for isValidCreateParams group request. Group name is required. Path
     * is optional.
     *
     * @param requestBody
     * @return
     */
    @Override
    public Boolean isValidCreateParams(Map<String, String> requestBody) {
        if (requestBody.containsKey("Path")) {
            if (!S3ParameterValidatorUtil.isValidPath(
                    requestBody.get("Path"))) {
                return false;
            }
        }

        return S3ParameterValidatorUtil.isValidGroupName(
                requestBody.get("GroupName"));
    }
}
