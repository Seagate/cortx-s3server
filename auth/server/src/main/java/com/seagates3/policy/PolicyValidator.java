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

import java.util.List;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.amazonaws.auth.policy.Action;
import com.amazonaws.auth.policy.Condition;
import com.amazonaws.auth.policy.Principal;
import com.amazonaws.auth.policy.Resource;
import com.amazonaws.auth.policy.Statement;
import com.amazonaws.auth.policy.Statement.Effect;
import com.amazonaws.auth.policy.conditions.ArnCondition;
import com.amazonaws.auth.policy.conditions.DateCondition;
import com.amazonaws.auth.policy.conditions.IpAddressCondition;
import com.amazonaws.auth.policy.conditions.NumericCondition;
import com.amazonaws.auth.policy.conditions.StringCondition;
import com.seagates3.dao.ldap.AccountImpl;
import com.seagates3.dao.ldap.UserImpl;
import com.seagates3.exception.DataAccessException;
import com.seagates3.model.Account;
import com.seagates3.model.User;
import com.seagates3.response.ServerResponse;
import com.seagates3.response.generator.PolicyResponseGenerator;

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

  abstract ServerResponse validatePolicy(String inputBucket, String jsonPolicy);

  /**
   * Validate if the Effect value is one of - Allow/Deny
   *
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
   *
   * @param conditionList
   * @return {@link ServerResponse}
   */
 protected
  ServerResponse validateCondition(List<Condition> conditionList) {
    ServerResponse response = null;
    if (conditionList != null) {
      for (Condition condition : conditionList) {
        String invalidConditionType =
            checkAndGetInvalidConditionType(condition);
        if (invalidConditionType != null) {
          response = responseGenerator.malformedPolicy(
              "Invalid Condition type : " + invalidConditionType);
          LOGGER.error("Condition type is invalid in bucket policy");
          break;
        }
      }
    }
    return response;
  }

 private
  String checkAndGetInvalidConditionType(Condition condition) {
    String conditionType = condition.getType();
    if (ArnCondition.ArnComparisonType.valueOf(conditionType) != null) {
      conditionType = null;
    } else if (StringCondition.StringComparisonType.valueOf(conditionType) !=
               null) {
      conditionType = null;
    } else if (NumericCondition.NumericComparisonType.valueOf(conditionType) !=
               null) {
      conditionType = null;
    } else if (DateCondition.DateComparisonType.valueOf(conditionType) !=
               null) {
      conditionType = null;
    } else if (IpAddressCondition.IpAddressComparisonType.valueOf(
                   conditionType) != null) {
      conditionType = null;
    }
    return conditionType;
  }

 protected
  ServerResponse validatePrincipal(List<Principal> principals) {
    ServerResponse response = null;
    if (principals != null && !principals.isEmpty()) {
      for (Principal principal : principals) {
        if (!principal.getProvider().equals("AWS") &&
            !principal.getProvider().equals("CanonicalUser") &&
            !principal.getProvider().equals("Federated") &&
            !principal.getProvider().equals("*")) {

          response =
              responseGenerator.malformedPolicy("Invalid bucket policy syntax");
          LOGGER.error("Invalid bucket policy syntax");
          break;
        }
        if (!"*".equals(principal.getId()) && !isPrincipalIdValid(principal)) {
          response =
              responseGenerator.malformedPolicy("Invalid principal in policy");
          LOGGER.error("Invalid principal in policy");
          break;
        }
      }
    } else {
      response =
          responseGenerator.malformedPolicy("Missing required field Principal");
      LOGGER.error("Missing required field Principal");
    }
    return response;
  }

 private
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
            if (!user.exists()) {
              isValid = false;
            }
          } else {
            account = accountImpl.findByID(id);
            if (!account.exists()) {
              user = userImpl.findByUserId(id);
              if (!user.exists()) {
                isValid = false;
              }
            }
          }
        }
        catch (DataAccessException e) {
          LOGGER.error("Exception occurred while finding principal", e);
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
   * Validate Action and Resource respectively first. Then validate each
   *Resource
   * against the Actions.
   *
   * @param actionList
   * @param resourceValues
   * @param inputBucket
   * @return {@link ServerResponse}
   */
  abstract ServerResponse
      validateActionAndResource(List<Action> actionList,
                                List<Resource> resourceValues,
                                String inputBucket);
}
