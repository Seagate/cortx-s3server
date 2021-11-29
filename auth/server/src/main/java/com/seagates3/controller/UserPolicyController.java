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
import com.seagates3.exception.GuardClauseException;
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
		ServerResponse serverResponse = null;

		try {
			User user = userDAO.find(requestor.getAccount().getName(), userName);
			checkIfUserExists(user, userName);

			Policy policy = policyDAO.findByArn(policyARN, requestor.getAccount());
			checkIfPolicyExists(policy, policyARN);
			checkIfPolicyIsAttached(user, policy, userName, policyARN);
			checkIfPolicyIsAttachable(policy, userName, policyARN);
			checkIfAttachPolicyExceedsQuota(user, userName, policyARN);

			userPolicyDAO.attach(new UserPolicy(user, policy));

			serverResponse = userPolicyResponseGenerator.generateAttachUserPolicyResponse();
		} catch (DataAccessException ex) {
			serverResponse = userPolicyResponseGenerator.internalServerError();
		} catch (GuardClauseException grdClsEx) {
			serverResponse = grdClsEx.getServerResponse();
		}

		return serverResponse;
	}

	public ServerResponse detach() throws DataAccessException {
		String userName = requestBody.get("UserName");
		String policyARN = requestBody.get("PolicyArn");

		LOGGER.info("Detach policy: " + policyARN + " from user: " + userName);
		ServerResponse serverResponse = null;

		try {
			User user = userDAO.find(requestor.getAccount().getName(), userName);
			checkIfUserExists(user, userName);

			Policy policy = policyDAO.findByArn(policyARN, requestor.getAccount());
			checkIfPolicyExists(policy, policyARN);
			checkIfPolicyIsNotAttached(user, policy, userName, policyARN);

			userPolicyDAO.detach(new UserPolicy(user, policy));

			serverResponse = userPolicyResponseGenerator.generateDetachUserPolicyResponse();
		} catch (DataAccessException ex) {
			serverResponse = userPolicyResponseGenerator.internalServerError();
		} catch (GuardClauseException grdClsEx) {
			serverResponse = grdClsEx.getServerResponse();
		}

		return serverResponse;
	}

	private void checkIfUserExists(User user, String userName) throws GuardClauseException {
		if (user == null || !user.exists()) {
			LOGGER.error("User [" + userName + "] does not exists.");
			throw new GuardClauseException(userPolicyResponseGenerator.noSuchEntity());
		}
	}

	private void checkIfPolicyExists(Policy policy, String policyARN) throws GuardClauseException {
		if (policy == null || !policy.exists()) {
			LOGGER.error("Policy [" + policyARN + "] does not exists.");
			throw new GuardClauseException(userPolicyResponseGenerator.noSuchEntity());
		}
	}

	private void checkIfPolicyIsAttached(User user, Policy policy, String userName, String policyARN)
			throws GuardClauseException {
		boolean isPolicyAttached = user.getPolicyIds().contains(policy.getPolicyId());
		if (isPolicyAttached) {
			LOGGER.error("Policy [" + policyARN + "] is already attached to user [" + userName + "].");
			throw new GuardClauseException(userPolicyResponseGenerator.userPolicyAlreadyAttached());
		}
	}

	private void checkIfPolicyIsNotAttached(User user, Policy policy, String userName, String policyARN)
			throws GuardClauseException {
		boolean isPolicyAttached = user.getPolicyIds().contains(policy.getPolicyId());
		if (!isPolicyAttached) {
			LOGGER.error("Policy [" + policyARN + "] is not attached to the user [" + userName + "].");
			throw new GuardClauseException(userPolicyResponseGenerator.userPolicyAlreadyAttached());
		}
	}

	private void checkIfPolicyIsAttachable(Policy policy, String userName, String policyARN)
			throws GuardClauseException {
		if (!Boolean.getBoolean(policy.getIsPolicyAttachable())) {
			LOGGER.error("Cannot attach non-attachable policy [" + policyARN + "] to user [" + userName + "].");
			throw new GuardClauseException(userPolicyResponseGenerator.attachNonAttachablePolicy());
		}
	}

	private void checkIfAttachPolicyExceedsQuota(User user, String userName, String policyARN)
			throws GuardClauseException {
		if (user.getPolicyIds().size() == AuthServerConfig.USER_POLICY_ATTACH_QUOTA) {
			LOGGER.error("Cannot attach policy [" + policyARN + "] to user [" + userName
					+ "] as policy attach quota is exceeded.");
			throw new GuardClauseException(userPolicyResponseGenerator.userPolicyAttachQuotaViolation());
		}
	}

}
