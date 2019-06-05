/*
 * COPYRIGHT 2019 SEAGATE LLC
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
 * Original author: Abhilekh Mustapure <abhilekh.mustapure@seagate.com>
 * Original creation date: 22-May-2019
 */
package com.seagates3.controller;

import java.util.Map;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.seagates3.dao.DAODispatcher;
import com.seagates3.dao.DAOResource;
import com.seagates3.dao.UserDAO;
import com.seagates3.dao.UserLoginProfileDAO;
import com.seagates3.exception.DataAccessException;
import com.seagates3.model.Requestor;
import com.seagates3.model.User;
import com.seagates3.response.ServerResponse;
import com.seagates3.response.generator.UserLoginProfileResponseGenerator;
import com.seagates3.response.generator.UserResponseGenerator;

public
class UserLoginProfileController extends AbstractController {

 private
  final UserDAO userDAO;
 private
  final UserLoginProfileDAO userLoginProfileDAO;

 private
  final UserLoginProfileResponseGenerator userLoginProfileResponseGenerator;
 private
  final UserResponseGenerator userResponseGenerator;
 private
  final Logger LOGGER =
      LoggerFactory.getLogger(UserLoginProfileController.class.getName());

 public
  UserLoginProfileController(Requestor requestor,
                             Map<String, String> requestBody) {
    super(requestor, requestBody);

    userDAO = (UserDAO)DAODispatcher.getResourceDAO(DAOResource.USER);
    userLoginProfileDAO = (UserLoginProfileDAO)DAODispatcher.getResourceDAO(
        DAOResource.USER_LOGIN_PROFILE);
    userLoginProfileResponseGenerator = new UserLoginProfileResponseGenerator();
    userResponseGenerator = new UserResponseGenerator();
  }

  @Override public ServerResponse create() {
    User user;
    try {
      user = userDAO.find(requestor.getAccount().getName(),
                          requestBody.get("UserName"));
    }
    catch (DataAccessException ex) {
      return userResponseGenerator.internalServerError();
    }

    if (!user.exists()) {
      LOGGER.error("User [" + user.getName() + "] does not exists");
      return userResponseGenerator.noSuchEntity();
    } else {
      try {
        user.setPassword(requestBody.get("Password"));
        userLoginProfileDAO.save(user);
      }
      catch (DataAccessException ex) {
        return userLoginProfileResponseGenerator.internalServerError();
      }
    }

    return userLoginProfileResponseGenerator.generateCreateResponse(user);
  }

  /**
  * Below method will return login profile of the user requested
  */
  @Override public ServerResponse list() {
    User user = null;
    ServerResponse response = null;
    try {
      user = userDAO.find(requestor.getAccount().getName(),
                          requestBody.get("UserName"));
      if (user.exists() && user.getPassword() != null) {
        response = userLoginProfileResponseGenerator.generateGetResponse(user);
      } else {
        response = userLoginProfileResponseGenerator.noSuchEntity();
      }
    }
    catch (DataAccessException ex) {
      response = userResponseGenerator.internalServerError();
    }
    LOGGER.debug("Returned response is  - " + response);
    return response;
  }
}
