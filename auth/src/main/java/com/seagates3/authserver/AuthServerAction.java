 /*
 * COPYRIGHT 2015 SEAGATE LLC
 *
 * THIS DRAWING/DOCUMENT, ITS SPECIFICATIONS, AND THE DATA CONTAINED
 * HEREIN, ARE THE EXCLUSIVE PROPERTY OF SEAGATE TECHNOLOGY
 * LIMITED, ISSUED IN STRICT CONFIDENCE AND SHALL NOT, WITHOUT
 * THE PRIOR WRITTEN PERMISSION OF SEAGATE TECHNOLOGY LIMITED,
 * BE REPRODUCED, COPIED, OR DISCLOSED TO A THIRD PARTY, OR
 * USED FOR ANY PURPOSE WHATSOEVER, OR STORED IN A RETRIEVAL SYSTEM
 * EXCEPT AS ALLOWED BY THE TERMS OF SEAGATE LICENSES AND AGREEMENTS.
 *
 * YOU SHOULD HAVE RECEIVED A COPY OF SEAGATE'S LICENSE ALONG WITH
 * THIS RELEASE. IF NOT PLEASE CONTACT A SEAGATE REPRESENTATIVE
 * http://www.seagate.com/contact
 *
 * Original author:  Arjun Hariharan <arjun.hariharan@seagate.com>
 * Original creation date: 17-Sep-2014
 */
package com.seagates3.authserver;

import com.seagates3.aws.request.ClientRequestParser;
import com.seagates3.aws.sign.SignatureValidator;
import com.seagates3.dao.AccessKeyDAO;
import com.seagates3.dao.DAODispatcher;
import com.seagates3.dao.DAOResource;
import com.seagates3.dao.RequestorDAO;
import com.seagates3.exception.DataAccessException;
import com.seagates3.model.AccessKey;
import com.seagates3.model.ClientRequestToken;
import com.seagates3.model.Requestor;
import com.seagates3.perf.S3Perf;
import com.seagates3.response.ServerResponse;
import com.seagates3.response.generator.AuthenticationResponseGenerator;
import com.seagates3.util.DateUtil;
import io.netty.handler.codec.http.FullHttpRequest;
import io.netty.handler.codec.http.HttpResponseStatus;
import java.lang.reflect.Constructor;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.HashMap;
import java.util.Map;
import org.joda.time.DateTime;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class AuthServerAction {

    private final Logger LOGGER = LoggerFactory.getLogger(
            AuthServerAction.class.getName());

    private final String VALIDATOR_PACKAGE = "com.seagates3.parameter.validator";
    private final String CONTROLLER_PACKAGE = "com.seagates3.controller";

    private S3Perf perf;

    AuthenticationResponseGenerator responseGenerator;

    public AuthServerAction() {
        responseGenerator = new AuthenticationResponseGenerator();
        perf = new S3Perf();
    }

    /*
     * Authenticate the requestor first.
     * If the requestor is authenticated, then perform the requested action.
     */
    public ServerResponse serve(FullHttpRequest httpRequest,
            Map<String, String> requestBody) {

        String requestAction = requestBody.get("Action");
        ClientRequestToken clientRequestToken;
        Requestor requestor;
        ServerResponse serverResponse;

        if (!requestAction.equals("CreateAccount")
                && !requestAction.equals("AssumeRoleWithSAML")) {
            LOGGER.debug("Parsing Client Request");
            clientRequestToken = ClientRequestParser.parse(httpRequest,
                    requestBody);

            /*
             * Client Request Token will be null if the request is incorrect.
             */
            if (clientRequestToken == null) {
                return responseGenerator.badRequest();
            }

            AccessKeyDAO accessKeyDAO = (AccessKeyDAO) DAODispatcher
                    .getResourceDAO(DAOResource.ACCESS_KEY);
            AccessKey accessKey;
            try {
                perf.startClock();
                accessKey = accessKeyDAO.find(clientRequestToken.getAccessKeyId());
                perf.endClock();
                perf.printTime("Fetch access key");
            } catch (DataAccessException ex) {
                LOGGER.error("Error occured while searching for requestor's "
                        + "access key id.\n" + ex.getMessage());
                return responseGenerator.internalServerError();
            }

            serverResponse = validateAccessKey(accessKey);
            if (serverResponse.getResponseStatus() != HttpResponseStatus.OK) {
                LOGGER.debug("Access key is invalid.\n"
                        + serverResponse.getResponseBody());

                return serverResponse;
            }

            LOGGER.debug("Access Key is valid");

            RequestorDAO requestorDAO = (RequestorDAO) DAODispatcher
                    .getResourceDAO(DAOResource.REQUESTOR);
            try {
                perf.startClock();
                requestor = requestorDAO.find(accessKey);
                perf.endClock();
                perf.printTime("Fetch requestor");
            } catch (DataAccessException ex) {
                return responseGenerator.internalServerError();
            }

            serverResponse = validateRequestor(requestor, clientRequestToken);
            if (serverResponse.getResponseStatus() != HttpResponseStatus.OK) {
                LOGGER.debug("Invalid requestor.\n"
                        + serverResponse.getResponseBody());

                return serverResponse;
            }

            LOGGER.debug("Requestor is valid.");
            LOGGER.debug("Calling signature validator.");

            perf.startClock();
            serverResponse = new SignatureValidator().validate(
                    clientRequestToken, requestor);
            perf.endClock();
            perf.printTime("Request validation");

            if (!serverResponse.getResponseStatus().equals(HttpResponseStatus.OK)) {
                LOGGER.debug("Incorrect signature.Request not authenticated");
                return serverResponse;
            }

            if (requestAction.equals("AuthenticateUser")) {
                serverResponse = responseGenerator.generateAuthenticatedResponse(
                        requestor, clientRequestToken);

                LOGGER.debug("Request is authenticated. Authenticate user "
                        + "response - " + serverResponse.getResponseBody());

                return serverResponse;
            }
        } else {
            requestor = new Requestor();
        }

        Map<String, String> controllerAction = getControllerAction(requestAction);
        String controllerName = controllerAction.get("ControllerName");
        String action = controllerAction.get("Action");

        LOGGER.debug("Controller name -" + controllerName);
        LOGGER.debug("Action name - " + action);

        if (!validateRequest(controllerName, action, requestBody)) {
            LOGGER.debug("Input parameters are not valid." + action);
            return responseGenerator.invalidParametervalue();
        }

        return performAction(controllerName, action, requestBody, requestor);
    }

    /**
     * Validate access Key.
     *
     * Check if access key exists. if access key is active.
     */
    private ServerResponse validateAccessKey(AccessKey accessKey) {
        /*
         * Return exit message if access key doesnt exist.
         */
        if (!accessKey.exists()) {
            LOGGER.debug("Access key doesn't exist.");
            return responseGenerator.invalidAccessKey();
        }

        if (!accessKey.isAccessKeyActive()) {
            LOGGER.debug("Access key in not active.");
            return responseGenerator.inactiveAccessKey();
        }

        return responseGenerator.ok();
    }

    /*
     * Validate the requestor.
     *
     * Check
     *   if requestor exists.
     *   if the requestor is a federated user, check if the access key is valid.
     */
    private ServerResponse validateRequestor(Requestor requestor,
            ClientRequestToken clientRequestToken) {
        /*
         * Return internal server error if requestor doesn't exist.
         *
         * Ideally, an access key will be associated with a requestor.
         * It is a server error if the access key doesn't belong to a user.
         */

        /**
         * TODO - AWS supports an error message called "TokenRefreshRequired".
         * Use case - If the creds gets expired in the middle of a huge file
         * upload, this token can be issued.
         *
         */
        if (!requestor.exists()) {
            LOGGER.debug("Requestor doesn't exist.");
            return responseGenerator.internalServerError();
        }

        AccessKey accessKey = requestor.getAccesskey();
        if (requestor.isFederatedUser()) {
            LOGGER.debug("Requestor is using federated credentials.");

            String sessionToken = clientRequestToken.getRequestHeaders()
                    .get("X-Amz-Security-Token");
            if (!accessKey.getToken().equals(sessionToken)) {
                LOGGER.debug("Invalid clientt token ID.");
                return responseGenerator.invalidClientTokenId();
            }

            DateTime currentDate = DateUtil.getCurrentDateTime();
            DateTime expiryDate = DateUtil.toDateTime(accessKey.getExpiry());

            if (currentDate.isAfter(expiryDate)) {
                LOGGER.debug("Federated credentials have expired.");
                return responseGenerator.expiredCredential();
            }
        }

        return responseGenerator.ok();
    }

    /*
     * Validate request parameters.
     */
    private Boolean validateRequest(String controllerName, String action,
            Map<String, String> requestBody) {
        Boolean isValidrequest = false;
        Class<?> validator;
        Method method;
        Object obj;
        String validatorClass = toValidatorClassName(controllerName);

        try {
            LOGGER.debug("Calling " + action + " validator.");
            validator = Class.forName(validatorClass);
            obj = validator.newInstance();
            method = validator.getMethod(action, Map.class);
            isValidrequest = (Boolean) method.invoke(obj, requestBody);
        } catch (ClassNotFoundException | NoSuchMethodException | SecurityException ex) {
            System.out.println(ex);
        } catch (IllegalAccessException | IllegalArgumentException |
                InvocationTargetException | InstantiationException ex) {
            System.out.println(ex);
        }

        return isValidrequest;
    }

    /*
     * Perform action requested by user.
     */
    private ServerResponse performAction(String controllerName, String action,
            Map<String, String> requestBody, Requestor requestor) {

        String controllerClassName = toControllerClassName(controllerName);
        Class<?> controller;
        Constructor<?> controllerConstructor;
        Method method;
        Object obj;

        try {
            LOGGER.debug("Calling " + action + " controller.");
            controller = Class.forName(controllerClassName);
            controllerConstructor = controller.getConstructor(Requestor.class, Map.class);
            obj = controllerConstructor.newInstance(requestor, requestBody);

            method = controller.getMethod(action);
            return (ServerResponse) method.invoke(obj);

        } catch (ClassNotFoundException | NoSuchMethodException | SecurityException ex) {
            System.out.println(ex);
        } catch (IllegalAccessException | IllegalArgumentException |
                InvocationTargetException | InstantiationException ex) {
            System.out.println(ex);
        }

        return null;
    }

    /*
     * Break the request action into the corresponding controller and action.
     */
    private Map<String, String> getControllerAction(String requestAction) {
        String pattern = "(?<=[a-z])(?=[A-Z])";
        String[] tokens = requestAction.split(pattern, 2);

        tokens[0] = tokens[0].toLowerCase();

        Map<String, String> controllerAction = new HashMap<>();

        /*
         * TODO
         * replace this entire logic with a mapper class.
         * ex -
         * createuser -> action = create, Controller = com.seagates3.controller.user
         */
        if ("assumerolewithsaml".equals(requestAction.toLowerCase())) {
            controllerAction.put("Action", "create");
            controllerAction.put("ControllerName", requestAction);
            return controllerAction;
        }

        if ("get".equals(tokens[0])) {
            controllerAction.put("Action", "create");
        } else {
            controllerAction.put("Action", tokens[0]);
        }

        if ((tokens[0].compareTo("list") == 0) && (tokens[1].endsWith("s"))) {
            controllerAction.put("ControllerName", tokens[1].substring(0, tokens[1].length() - 1));
        } else if (tokens[0].compareTo("authenticate") == 0) {
            controllerAction.put("ControllerName", "requestor");
        } else {
            controllerAction.put("ControllerName", tokens[1]);
        }

        return controllerAction;
    }

    /**
     * Return the full name of the controller class.
     */
    private String toControllerClassName(String controllerName) {
        return String.format("%s.%sController", CONTROLLER_PACKAGE, controllerName);
    }

    /**
     * Return the full name of the validator class.
     */
    private String toValidatorClassName(String controllerName) {
        return String.format("%s.%sParameterValidator", VALIDATOR_PACKAGE,
                controllerName);
    }
}
