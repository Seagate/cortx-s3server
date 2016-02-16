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
 * Original creation date: 13-Nov-2015
 */
package com.seagates3.authserver;

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import org.junit.Assert;
import org.junit.Test;

public class AuthServerActionTest {

    AuthServerAction authServerAction;

    public AuthServerActionTest() {
        authServerAction = new AuthServerAction();
    }

    /**
     * Test AuthServerAction#toControllerClassName. Case - Check if valid
     * controller class name is returned.
     *
     * @throws java.lang.NoSuchMethodException
     * @throws java.lang.IllegalAccessException
     * @throws java.lang.reflect.InvocationTargetException
     */
    @Test
    public void ToControllerClassName_ControllerName_GetClassName()
            throws NoSuchMethodException, IllegalAccessException,
            IllegalArgumentException, InvocationTargetException {
        Class[] parameterTypes = new Class[1];
        parameterTypes[0] = String.class;
        Method private_method = authServerAction.getClass().getDeclaredMethod(
                "toControllerClassName", parameterTypes);
        private_method.setAccessible(true);

        Object[] parameters = new Object[1];
        parameters[0] = "User";

        String expectedClassName = "com.seagates3.controller.UserController";
        String className = (String) private_method.invoke(authServerAction, parameters);
        Assert.assertEquals(expectedClassName, className);
    }

    /**
     * Test AuthServerAction#toValidatorClassName. Case - Check if valid
     * validator class name is returned.
     *
     * @throws java.lang.NoSuchMethodException
     * @throws java.lang.IllegalAccessException
     * @throws java.lang.reflect.InvocationTargetException
     */
    @Test
    public void ToValidatorClassName_ControllerName_GetClassName()
            throws NoSuchMethodException, IllegalAccessException,
            IllegalArgumentException, InvocationTargetException {
        Class[] parameterTypes = new Class[1];
        parameterTypes[0] = String.class;
        Method private_method = authServerAction.getClass().getDeclaredMethod(
                "toValidatorClassName", parameterTypes);
        private_method.setAccessible(true);

        Object[] parameters = new Object[1];
        parameters[0] = "User";

        String expectedClassName = "com.seagates3.parameter.validator.UserParameterValidator";
        String className = (String) private_method.invoke(authServerAction, parameters);
        Assert.assertEquals(expectedClassName, className);
    }
}
