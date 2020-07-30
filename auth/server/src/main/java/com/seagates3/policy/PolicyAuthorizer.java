package com.seagates3.policy;

import java.util.Map;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.seagates3.model.Requestor;
import com.seagates3.response.ServerResponse;

public
abstract class PolicyAuthorizer extends PolicyAuthorizerConstants {

 protected
  final Logger LOGGER =
      LoggerFactory.getLogger(PolicyAuthorizer.class.getName());

 public
  abstract ServerResponse
      authorizePolicy(Requestor requestor, Map<String, String> requestBody);
}
