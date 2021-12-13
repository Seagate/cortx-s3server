package com.seagates3.dao.ldap;

import static org.junit.Assert.assertTrue;
import static org.mockito.Matchers.any;
import static org.mockito.Matchers.anyString;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.powermock.api.mockito.PowerMockito;
import org.powermock.core.classloader.annotations.PowerMockIgnore;
import org.powermock.core.classloader.annotations.PrepareForTest;
import org.powermock.modules.junit4.PowerMockRunner;

import com.novell.ldap.LDAPException;
import com.novell.ldap.LDAPModification;
import com.seagates3.exception.DataAccessException;
import com.seagates3.model.Policy;
import com.seagates3.model.User;
import com.seagates3.model.UserPolicy;

@RunWith(PowerMockRunner.class) @PrepareForTest({LDAPUtils.class})
    @PowerMockIgnore(
        {"javax.management.*"}) public class UserPolicyLdapStoreTest {

 private
  static final String USER_ID = "AAABBBCCCDDD";
 private
  static final String USER_NAME = "iamuser1";
 private
  static final String POLICY_ID = "EEEFFFGGGHHH";

 private
  UserPolicy userPolicy;

  @Before public void setUp() throws Exception {
    PowerMockito.mockStatic(LDAPUtils.class);

    userPolicy = new UserPolicy();
    userPolicy.setPolicy(buildPolicy());
    userPolicy.setUser(buildUser());
  }

  @Test public void testAttachSuccess() throws Exception {
    PowerMockito.doNothing().when(LDAPUtils.class, "modify", "",
                                  new LDAPModification());
    UserPolicyLdapStore userPolicyLdapStore = new UserPolicyLdapStore();
    userPolicyLdapStore.attach(userPolicy);
    assert(true);
  }

  @Test public void testDetachSuccess() throws Exception {
    PowerMockito.doNothing().when(LDAPUtils.class, "modify", "",
                                  new LDAPModification());
    UserPolicyLdapStore userPolicyLdapStore = new UserPolicyLdapStore();
    userPolicyLdapStore.detach(userPolicy);
    assert(true);
  }

  @Test public void testAttachFailed() throws Exception {
    UserPolicyLdapStore userPolicyLdapStore =
        buildUserPolicyLdapStoreObjToThrowEx();

    try {
      userPolicyLdapStore.attach(userPolicy);
      assert(false);
    }
    catch (DataAccessException ex) {
      String errMsgPart = "Failed to attach policy [" + POLICY_ID +
                          "] to user [" + USER_NAME + "]";
      assertTrue("Unexpected exception message.",
                 ex.getMessage().contains(errMsgPart));
    }
  }

  @Test public void testDetachFailed() throws Exception {
    UserPolicyLdapStore userPolicyLdapStore =
        buildUserPolicyLdapStoreObjToThrowEx();

    try {
      userPolicyLdapStore.detach(userPolicy);
      assert(false);
    }
    catch (DataAccessException ex) {
      String errMsgPart = "Failed to detach policy [" + POLICY_ID +
                          "] from user [" + USER_NAME + "]";
      assertTrue("Unexpected exception message.",
                 ex.getMessage().contains(errMsgPart));
    }
  }

 private
  UserPolicyLdapStore buildUserPolicyLdapStoreObjToThrowEx()
      throws LDAPException {
    PowerMockito.doThrow(new LDAPException()).when(LDAPUtils.class);
    LDAPUtils.modify(anyString(), any(LDAPModification.class));
    UserPolicyLdapStore userPolicyLdapStore = new UserPolicyLdapStore();

    return userPolicyLdapStore;
  }

 private
  User buildUser() {
    User user = new User();
    user.setId(USER_ID);
    user.setName(USER_NAME);

    return user;
  }

 private
  Policy buildPolicy() {
    Policy policy = new Policy();
    policy.setPolicyId(POLICY_ID);
    policy.setIsPolicyAttachable("true");

    return policy;
  }
}
