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

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;

import java.util.Map;
import java.util.HashMap;

import io.netty.handler.codec.http.HttpRequest;
import io.netty.handler.codec.http.HttpResponseStatus;

import com.seagates3.awssign.RequestorAuthenticator;
import com.seagates3.dao.AccessKeyDAO;
import com.seagates3.dao.DAODispatcher;
import com.seagates3.dao.DAOResource;
import com.seagates3.dao.RequestorDAO;
import com.seagates3.model.AccessKey;
import com.seagates3.model.Requestor;
import com.seagates3.response.ServerResponse;
import com.seagates3.response.generator.xml.AuthenticationResponseGenerator;
import com.seagates3.response.generator.xml.XMLResponseGenerator;
import java.lang.reflect.Constructor;

public class AuthServerAction {

    private final String VALIDATOR_PACKAGE = "com.seagates3.validator";
    private final String CONTROLLER_PACKAGE = "com.seagates3.controller";

    /*
     * Authenticate the requestor first.
     * If the requestor is authenticated, then perform the requested action.
     */
    public ServerResponse serve(HttpRequest httpRequest,
            Map<String, String> requestBody) {

        String requestAction = requestBody.get("Action");
        ClientRequest request;
        Requestor requestor;

        if( !requestAction.equals("CreateAccount")) {
            if(requestAction.equals("AuthenticateUser")) {
                request = new ClientRequest(requestBody);
            } else {
                request = new ClientRequest(httpRequest);
            }

            AccessKeyDAO accessKeyDAO = (AccessKeyDAO)
                        DAODispatcher.getResourceDAO(DAOResource.ACCESS_KEY);
            AccessKey accessKey = accessKeyDAO.findAccessKey(request.getAccessKeyId());

            RequestorDAO requestorDAO = (RequestorDAO)
                    DAODispatcher.getResourceDAO(DAOResource.REQUESTOR);
            requestor = requestorDAO.findRequestor(accessKey);

            ServerResponse serverResponse =
                    RequestorAuthenticator.authenticate(requestor, request);

            if(!serverResponse.getResponseStatus().equals(HttpResponseStatus.OK)) {
                return serverResponse;
            }

            /* To do
             * Replace this with requestor controller
             */
            if(requestAction.equals("AuthenticateUser")) {
                return new AuthenticationResponseGenerator().AuthenticateUser();
            }
        } else {
            requestor = new Requestor();
        }

        Map<String, String> controllerAction = getControllerAction(requestAction);
        String controllerName = controllerAction.get("ControllerName");
        String action = controllerAction.get("Action");

        if(! validRequest(controllerName, action, requestBody)) {
            return new XMLResponseGenerator().invalidParametervalue();
        }

        return performAction(controllerName, action, requestBody, requestor);
    }


    private Boolean validRequest(String controllerName, String action,
            Map<String, String> requestBody) {
        Boolean isValidrequest = false;
        Class<?> validator;
        Method method;
        Object obj;
        String validatorClass = toValidatorClass(controllerName);

        try {
            validator = Class.forName(validatorClass);
            obj = validator.newInstance();
            method = validator.getMethod(action, Map.class);
            isValidrequest = (Boolean) method.invoke (obj, requestBody);
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

        String controllerClassName = toControllerClass(controllerName);
        Class<?> controller;
        Constructor<?> controllerConstructor;
        Method method;
        Object obj;

        try {
            controller = Class.forName(controllerClassName);
            controllerConstructor = controller.getConstructor(Requestor.class, Map.class);
            obj = controllerConstructor.newInstance(requestor, requestBody);

            method = controller.getMethod(action);
            return (ServerResponse) method.invoke (obj);

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
        String pattern  = "(?<=[a-z])(?=[A-Z])";
        String[] tokens = requestAction.split(pattern, 2);

        Map<String, String> controllerAction = new HashMap<>();

        if("Get".equals(tokens[0])) {
            controllerAction.put("Action", "create");
        } else {
            controllerAction.put("Action", tokens[0].toLowerCase());
        }

        if((tokens[0].compareTo("list") == 0) && (tokens[1].endsWith("s"))) {
            controllerAction.put("ControllerName", tokens[1].substring(0, tokens[1].length() -1));
        } else if (tokens[0].compareTo("authenticate") == 0) {
            controllerAction.put("ControllerName", "requestor");
        } else {
            controllerAction.put("ControllerName", tokens[1]);
        }

        return controllerAction;
    }

    private String toControllerClass(String controllerName) {
        return String.format("%s.%sController", CONTROLLER_PACKAGE, controllerName);
    }

    private String toValidatorClass(String controllerName) {
        return String.format("%s.%sValidator", VALIDATOR_PACKAGE, controllerName);
    }
}
