package com.seagates3.controller;

import java.util.Map;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.seagates3.authserver.AuthServerConfig;
import com.seagates3.dao.DAODispatcher;
import com.seagates3.dao.DAOResource;
import com.seagates3.dao.PolicyDAO;
import com.seagates3.dao.UserDAO;
import com.seagates3.dao.UserPolicyDAO;
import com.seagates3.exception.DataAccessException;
import com.seagates3.model.Policy;
import com.seagates3.model.Requestor;
import com.seagates3.model.User;
import com.seagates3.model.UserPolicy;
import com.seagates3.response.ServerResponse;
import com.seagates3.response.generator.UserPolicyResponseGenerator;

public class UserPolicyController extends AbstractController {

	private final Logger LOGGER = LoggerFactory.getLogger(UserPolicyController.class.getName());

	private final UserDAO userDAO;
	private final PolicyDAO policyDAO;
	private final UserPolicyDAO userPolicyDAO;
	private final UserPolicyResponseGenerator userPolicyResponseGenerator;

	public UserPolicyController(Requestor requestor, Map<String, String> requestBody) {
		super(requestor, requestBody);

		userDAO = (UserDAO) DAODispatcher.getResourceDAO(DAOResource.USER);
		policyDAO = (PolicyDAO) DAODispatcher.getResourceDAO(DAOResource.POLICY);
		userPolicyDAO = (UserPolicyDAO) DAODispatcher.getResourceDAO(DAOResource.USER_POLICY);
		userPolicyResponseGenerator = new UserPolicyResponseGenerator();
	}

	public ServerResponse attach() throws DataAccessException {
		String userName = requestBody.get("UserName");
		String policyARN = requestBody.get("PolicyArn");

		LOGGER.info("Attach policy: " + policyARN + " to user: " + userName);
		User user = null;
		try {
			user = userDAO.find(requestor.getAccount().getName(), userName);
		} catch (DataAccessException ex) {
			LOGGER.error("Exception occurred when finding the user [" + userName + "].");
			return userPolicyResponseGenerator.internalServerError();
		}

		if (!user.exists()) {
			LOGGER.error("User [" + user.getName() + "] does not exists.");
			return userPolicyResponseGenerator.noSuchEntity();
		}

		Policy policy = null;
		try {
			policy = policyDAO.findByArn(policyARN, requestor.getAccount());
		} catch (DataAccessException ex) {
			LOGGER.error("Failed to find the requested policy in ldap- " + policyARN);
			return userPolicyResponseGenerator.internalServerError();
		}
		if (policy == null || !policy.exists()) {
			LOGGER.error("Policy [" + policyARN + "] does not exists.");
			return userPolicyResponseGenerator.noSuchEntity();
		}
		boolean isPolicyAttached = user.getPolicyIds().contains(policy.getPolicyId());
		if (isPolicyAttached) {
			LOGGER.error("Policy [" + policyARN + "] is already attached to user [" + userName + "].");
			return userPolicyResponseGenerator.userPolicyAlreadyAttached();
		}
		if (!Boolean.getBoolean(policy.getIsPolicyAttachable())) {
			LOGGER.error("Cannot attach non-attachable policy [" + policyARN + "] to user [" + userName + "].");
			return userPolicyResponseGenerator.attachNonAttachablePolicy();
		}
		if (user.getPolicyIds().size() == AuthServerConfig.USER_POLICY_ATTACH_QUOTA) {
			LOGGER.error("Cannot attach policy [" + policyARN + "] to user [" + userName
					+ "] as policy attach quota is exceeded.");
			return userPolicyResponseGenerator.userPolicyAttachQuotaViolation();
		}
		try {
			userPolicyDAO.attach(new UserPolicy(user, policy));
		} catch (DataAccessException ex) {
			LOGGER.error("Exception while attaching the policy [" + policyARN + "] to user [" + userName + "].");
			return userPolicyResponseGenerator.internalServerError();
		}

		return userPolicyResponseGenerator.generateAttachUserPolicyResponse();
	}

	public ServerResponse detach() throws DataAccessException {
		String userName = requestBody.get("UserName");
		String policyARN = requestBody.get("PolicyArn");

		LOGGER.info("Detach policy: " + policyARN + " from user: " + userName);
		User user;
		try {
			user = userDAO.find(requestor.getAccount().getName(), userName);
		} catch (DataAccessException ex) {
			LOGGER.error("Exception occurred when finding the user [" + userName + "].");
			return userPolicyResponseGenerator.internalServerError();
		}

		if (!user.exists()) {
			LOGGER.error("User [" + user.getName() + "] does not exists.");
			return userPolicyResponseGenerator.noSuchEntity();
		}

		Policy policy = null;
		try {
			policy = policyDAO.findByArn(policyARN, requestor.getAccount());
		} catch (DataAccessException ex) {
			LOGGER.error("Failed to find the requested policy in ldap- " + policyARN);
			return userPolicyResponseGenerator.internalServerError();
		}
		if (policy == null || !policy.exists()) {
			LOGGER.error("Policy [" + policyARN + "] does not exists.");
			return userPolicyResponseGenerator.noSuchEntity();
		}
		boolean isPolicyAttached = user.getPolicyIds().contains(policy.getPolicyId());
		if (!isPolicyAttached) {
			LOGGER.error("Policy [" + policyARN + "] is not attached to the user [" + userName + "].");
			return userPolicyResponseGenerator.detachNonAttachedPolicy();
		}
		try {
			userPolicyDAO.detach(new UserPolicy(user, policy));
		} catch (DataAccessException ex) {
			LOGGER.error("Exception while detaching the policy [" + policyARN + "] from user [" + userName + "].");
			return userPolicyResponseGenerator.internalServerError();
		}

		return userPolicyResponseGenerator.generateDetachUserPolicyResponse();
	}

}
