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
import com.seagates3.response.ServerResponse;
import com.seagates3.response.generator.AuthenticationResponseGenerator;
import com.seagates3.util.DateUtil;
import io.netty.handler.codec.http.FullHttpRequest;
import io.netty.handler.codec.http.HttpResponseStatus;
import java.lang.reflect.Constructor;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.Date;
import java.util.HashMap;
import java.util.Map;

public class AuthServerAction {

    private final String VALIDATOR_PACKAGE = "com.seagates3.parameter.validator";
    private final String CONTROLLER_PACKAGE = "com.seagates3.controller";

    AuthenticationResponseGenerator responseGenerator;

    public AuthServerAction() {
        responseGenerator = new AuthenticationResponseGenerator();
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
                accessKey = accessKeyDAO.find(clientRequestToken.getAccessKeyId());
            } catch (DataAccessException ex) {
                return responseGenerator.internalServerError();
            }

            serverResponse = validateAccessKey(accessKey);
            if (serverResponse.getResponseStatus() != HttpResponseStatus.OK) {
                return serverResponse;
            }

            RequestorDAO requestorDAO = (RequestorDAO) DAODispatcher
                    .getResourceDAO(DAOResource.REQUESTOR);
            try {
                requestor = requestorDAO.find(accessKey);
            } catch (DataAccessException ex) {
                return responseGenerator.internalServerError();
            }

            serverResponse = validateRequestor(requestor, clientRequestToken);
            if (serverResponse.getResponseStatus() != HttpResponseStatus.OK) {
                return serverResponse;
            }

            serverResponse = new SignatureValidator().validate(
                    clientRequestToken, requestor);

            if (!serverResponse.getResponseStatus().equals(HttpResponseStatus.OK)) {
                return serverResponse;
            }

            if (requestAction.equals("AuthenticateUser")) {
                return responseGenerator.generateAuthenticatedResponse(requestor);
            }
        } else {
            requestor = new Requestor();
        }

        Map<String, String> controllerAction = getControllerAction(requestAction);
        String controllerName = controllerAction.get("ControllerName");
        String action = controllerAction.get("Action");

        if (!validateRequest(controllerName, action, requestBody)) {
            return responseGenerator.invalidParametervalue();
        }

        return performAction(controllerName, action, requestBody, requestor);
    }

    /*
     * Validate access Key.
     *
     * Check
     *   if access key exists.
     *   if access key is active.
     */
    private ServerResponse validateAccessKey(AccessKey accessKey) {
        /*
         * Return exit message if access key doesnt exist.
         */
        if (!accessKey.exists()) {
            return responseGenerator.noSuchEntity();
        }

        if (!accessKey.isAccessKeyActive()) {
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
        if (!requestor.exists()) {
            return responseGenerator.internalServerError();
        }

        AccessKey accessKey = requestor.getAccesskey();
        if (requestor.isFederatedUser()) {
            String sessionToken = clientRequestToken.getRequestHeaders()
                    .get("X-Amz-Security-Token");
            if (!accessKey.getToken().equals(sessionToken)) {
                return responseGenerator.invalidClientTokenId();
            }

            Date currentDate = new Date();
            Date expiryDate = DateUtil.toDate(accessKey.getExpiry());

            if (currentDate.after(expiryDate)) {
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
