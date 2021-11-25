package com.seagates3.controller;

import java.util.Map;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.seagates3.exception.DataAccessException;
import com.seagates3.model.Requestor;
import com.seagates3.response.ServerResponse;
import com.seagates3.response.generator.UserPolicyResponseGenerator;

public class UserPolicyController extends AbstractController {

	private final Logger LOGGER = LoggerFactory.getLogger(UserPolicyController.class.getName());
	
	private final UserPolicyResponseGenerator userPolicyResponseGenerator;

	public UserPolicyController(Requestor requestor, Map<String, String> requestBody) {
		super(requestor, requestBody);
		userPolicyResponseGenerator = new UserPolicyResponseGenerator();
	}
	
	public ServerResponse attach() throws DataAccessException {
		LOGGER.info("--------Attach Start------------");
		if (requestBody.get("UserName") != null) {
			LOGGER.info("User name: " + requestBody.get("UserName"));
	    }
		if (requestBody.get("PolicyArn") != null) {
			LOGGER.info("Policy ARN: " + requestBody.get("PolicyArn"));
	    }
		LOGGER.info("--------Attach End------------");
		return userPolicyResponseGenerator.generateAttachUserPolicyResponse();
	}
	
	public ServerResponse detach() throws DataAccessException {
		LOGGER.info("--------Detach Start------------");
		if (requestBody.get("UserName") != null) {
			LOGGER.info("User name: " + requestBody.get("UserName"));
	    }
		if (requestBody.get("PolicyArn") != null) {
			LOGGER.info("Policy ARN: " + requestBody.get("PolicyArn"));
	    }
		LOGGER.info("--------Detach End------------");
		return userPolicyResponseGenerator.generateDetachUserPolicyResponse();
	}

}
