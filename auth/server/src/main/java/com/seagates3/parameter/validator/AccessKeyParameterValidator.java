package com.seagates3.parameter.validator;

import java.util.Map;

/**
 * Validate the input for Access Key APIs - Create, Delete, List and isValidUpdateParams.
 */
public class AccessKeyParameterValidator extends AbstractParameterValidator {

    /**
     * Validate the input parameters for isValidCreateParams access key request.
     * User name is optional.
     *
     * @param requestBody TreeMap of input parameters.
     * @return true if input is valid.
     */
    @Override
    public Boolean isValidCreateParams(Map<String, String> requestBody) {
        if (requestBody.containsKey("UserName")) {
            return S3ParameterValidatorUtil.isValidName(requestBody.get("UserName"));
        }

        return true;
    }

    /**
     * Validate the input parameters for isValidDeleteParams access key request.
     * Access key id is required.
     * User name is optional.
     *
     * @param requestBody TreeMap of input parameters.
     * @return true if input is valid.
     */
    @Override
    public Boolean isValidDeleteParams(Map<String, String> requestBody) {
        if (requestBody.containsKey("UserName")) {
            return S3ParameterValidatorUtil.isValidName(requestBody.get("UserName"));
        }

        return S3ParameterValidatorUtil.isValidAccessKeyId(requestBody.get("AccessKeyId"));
    }

    /**
     * Validate the input parameters for isValidListParams access keys request.
     * Path prefix is optional.
     * Max Items is optional.
     * Marker is optional.
     *
     * @param requestBody TreeMap of input parameters.
     * @return true if input is valid.
     */
    @Override
    public Boolean isValidListParams(Map<String, String> requestBody) {
        if (requestBody.containsKey("UserName")) {
            if (!S3ParameterValidatorUtil.isValidName(requestBody.get("UserName"))) {
                return false;
            }
        }

        if (requestBody.containsKey("MaxItems")) {
            if (!S3ParameterValidatorUtil.isValidMaxItems(requestBody.get("MaxItems"))) {
                return false;
            }
        }

        if (requestBody.containsKey("Marker")) {
            return S3ParameterValidatorUtil.isValidMarker(requestBody.get("Marker"));
        }

        return true;
    }

    /**
     * Validate the input parameters for isValidUpdateParams access key request.
     * Access key id is required.
     * Status is required.
     * User Name is optional.
     *
     * @param requestBody TreeMap of input parameters.
     * @return true if input is valid.
     */
    @Override
    public Boolean isValidUpdateParams(Map<String, String> requestBody) {
        if (!S3ParameterValidatorUtil.isValidAccessKeyStatus(requestBody.get("Status"))) {
            return false;
        }

        if (requestBody.containsKey("UserName")) {
            if (!S3ParameterValidatorUtil.isValidName(requestBody.get("UserName"))) {
                return false;
            }
        }

        return S3ParameterValidatorUtil.isValidAccessKeyId(requestBody.get("AccessKeyId"));
    }
}
