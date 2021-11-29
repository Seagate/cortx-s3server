package com.seagates3.dao.ldap;

import java.util.HashMap;
import java.util.Map;

import com.seagates3.dao.UserPolicyDAO;
import com.seagates3.exception.DataAccessException;
import com.seagates3.model.UserPolicy;

public class UserPolicyImpl implements UserPolicyDAO {

	private static final String USER_POLICY_PREFIX = "UserPolicy";

	@Override
	public void attach(UserPolicy userPolicy) throws DataAccessException {
		AuthStore storeInstance = getAuthStoreInstance();
		Map userPolicyDetailsMap = new HashMap<>();
		userPolicyDetailsMap.put("", userPolicy);
		storeInstance.attach(userPolicyDetailsMap, USER_POLICY_PREFIX);
	}

	@Override
	public void detach(UserPolicy userPolicy) throws DataAccessException {
		AuthStore storeInstance = getAuthStoreInstance();
		Map userPolicyDetailsMap = new HashMap<>();
		userPolicyDetailsMap.put("", userPolicy);
		storeInstance.detach(userPolicyDetailsMap, USER_POLICY_PREFIX);
	}
	
	private AuthStore getAuthStoreInstance() {
		AuthStoreFactory factory = new AuthStoreFactory();
		AuthStore storeInstance = factory.createAuthStore(USER_POLICY_PREFIX);
		return storeInstance;
	} 

}
