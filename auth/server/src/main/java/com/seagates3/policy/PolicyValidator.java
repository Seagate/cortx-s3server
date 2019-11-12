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
 * Original author:  Ajinkya Dhumal
 * Original creation date: 10-November-2019
 */

package com.seagates3.policy;

import java.util.ArrayList;
import java.util.List;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.amazonaws.auth.policy.Action;
import com.amazonaws.auth.policy.Condition;
import com.amazonaws.auth.policy.Policy;
import com.amazonaws.auth.policy.Principal;
import com.amazonaws.auth.policy.Resource;
import com.amazonaws.auth.policy.Statement;
import com.amazonaws.auth.policy.Statement.Effect;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import com.seagates3.dao.ldap.AccountImpl;
import com.seagates3.dao.ldap.UserImpl;
import com.seagates3.exception.DataAccessException;
import com.seagates3.model.Account;
import com.seagates3.model.User;
import com.seagates3.response.ServerResponse;
import com.seagates3.response.generator.PolicyResponseGenerator;
import com.seagates3.util.BinaryUtil;

/**
 * Generic class to validate Policy. Sub classes of this class should be
 * instantiated in order to validate the policy.
 */
public
abstract class PolicyValidator {

 private
  final Logger LOGGER =
      LoggerFactory.getLogger(PolicyValidator.class.getName());

 protected
  PolicyResponseGenerator responseGenerator = null;

  /**
   * Validate Policy
   * @param inputBucket
   * @param jsonPolicy
   * @return {@link ServerResponse}
   */
 public
  ServerResponse validatePolicy(String inputBucket, String jsonPolicy) {

    ServerResponse response = null;
    JsonParser jsonParser = new JsonParser();
    JsonObject obj =
        (JsonObject)jsonParser.parse(BinaryUtil.base64DecodeString(jsonPolicy));
    Policy policy = null;
    try {
      policy = Policy.fromJson(obj.toString());
    }
    catch (Exception e) {
      response = responseGenerator.malformedPolicy(
          "This policy contains invalid Json");
      LOGGER.error("This policy contains invalid Json");
      return response;
    }
    // Statement validation
    if (policy.getStatements() != null) {
      List<Statement> statementList =
          new ArrayList<Statement>(policy.getStatements());
      for (Statement stmt : statementList) {
        // Action validation
        response = validateActionAndResource(stmt.getActions(),
                                             stmt.getResources(), inputBucket);
        if (response != null) break;

        // Effect validation
        response = validateEffect(stmt.getEffect());
        if (response != null) break;

        // condition validation
        response = validateCondition(stmt.getConditions());
        if (response != null) break;

        // Principal Validation
        response = validatePrincipal(stmt.getPrincipals());
        if (response != null) break;
      }
    } else {  // statement null
      response =
          responseGenerator.malformedPolicy("Missing required field Statement");
      LOGGER.error("Missing required field Statement");
    }
    return response;
  }

  /**
   * Validate if Principal form the Policy Statement is valid
   * @param principal
   * @return true if Principal is valid
   */
 protected
  boolean isPrincipalIdValid(Principal principal) {
    boolean isValid = true;
    String provider = principal.getProvider();
    String id = principal.getId();
    AccountImpl accountImpl = new AccountImpl();
    UserImpl userImpl = new UserImpl();
    Account account = null;
    switch (provider) {
      case "AWS":
        account = null;
        User user = null;
        try {
          if (new PrincipalArnParser().isArnFormatValid(id)) {
            user = userImpl.findByArn(id);
            if (user == null || !user.exists()) {
              isValid = false;
            }
          } else {
            account = accountImpl.findByID(id);
            if (account == null || !account.exists()) {
              user = userImpl.findByUserId(id);
              if (user == null || !user.exists()) {
                isValid = false;
              }
            }
          }
        }
        catch (DataAccessException e) {
          LOGGER.error("Exception occurred while finding account by account id",
                       e);
          isValid = false;
        }
        break;
      case "CanonicalUser":
        account = null;
        try {
          account = accountImpl.findByCanonicalID(id);
        }
        catch (DataAccessException e) {
          LOGGER.error(
              "Exception occurred while finding account by canonical id", e);
        }
        if (account == null || !account.exists()) isValid = false;
        break;
      case "Federated":
        // Not supporting as of now
        isValid = false;
        break;
    }
    return isValid;
  }

  /**
   * Validate if the Effect value is one of - Allow/Deny
   * @param effectValue
   * @return {@link ServerResponse}
   */
 protected
  ServerResponse validateEffect(Effect effectValue) {
    ServerResponse response = null;
    if (effectValue != null) {
      if (Statement.Effect.valueOf(effectValue.name()) == null) {
        response = responseGenerator.malformedPolicy("Invalid effect : " +
                                                     effectValue.name());
        LOGGER.error("Effect value is invalid in bucket policy");
      }
    } else {
      response =
          responseGenerator.malformedPolicy("Missing required field Effect");
      LOGGER.error("Missing required field Effect");
    }
    return response;
  }

  /**
   * Validate the Conditions form Statement
   * @param conditionList
   * @return {@link ServerResponse}
   */
  abstract ServerResponse validateCondition(List<Condition> conditionList);

  /**
   * Validate Action and Resource respectively first. Then validate each
   * Resource against the Actions.
   * @param actionList
   * @param resourceValues
   * @param inputBucket
   * @return {@link ServerResponse}
   */
  abstract ServerResponse
      validateActionAndResource(List<Action> actionList,
                                List<Resource> resourceValues,
                                String inputBucket);

  /**
   * Validate Principal from the Policy Statement
   * @param principals
   * @return {@link ServerResponse}
   */
  abstract ServerResponse validatePrincipal(List<Principal> principals);
}
