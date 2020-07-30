package com.seagates3.response.generator;

import java.util.ArrayList;
import java.util.LinkedHashMap;

import com.seagates3.authserver.AuthServerConfig;
import com.seagates3.model.Account;
import com.seagates3.response.ServerResponse;
import com.seagates3.response.formatter.xml.XMLResponseFormatter;
/**
 *
 * TODO : Remove this comment once below code tested end to end
 *
 */
public
class AccountLoginProfileResponseGenerator extends AbstractResponseGenerator {

  /**
     * This method will generate 'GetLoginProfile' response
     *
     * @param Account
     * @return LoginProfile XML of the requested account
  */
 public
  ServerResponse generateGetResponse(Account account) {
    LinkedHashMap<String, String> responseElements = new LinkedHashMap<>();
    ArrayList<LinkedHashMap<String, String>> accountAttr = new ArrayList<>();
    responseElements.put("AccountName", account.getName());
    responseElements.put("CreateDate", account.getProfileCreateDate());
    responseElements.put("PasswordResetRequired",
                         account.getPwdResetRequired().toLowerCase());
    accountAttr.add(responseElements);
    return new XMLResponseFormatter().formatGetResponse(
        "GetAccountLoginProfile", "LoginProfile", accountAttr,
        AuthServerConfig.getReqId());
  }

  /**
     * This method will generate 'CreateAccountLoginProfile' response
     *
     * @param Account
     * @return LoginProfile XML of the requested account
  */

 public
  ServerResponse generateCreateResponse(Account account) {
    LinkedHashMap<String, String> responseElements =
        new LinkedHashMap<String, String>();
    responseElements.put("AccountName", account.getName());
    responseElements.put("PasswordResetRequired",
                         account.getPwdResetRequired().toLowerCase());
    responseElements.put("CreateDate", account.getProfileCreateDate());
    return new XMLResponseFormatter().formatCreateResponse(
        "CreateAccountLoginProfile", "LoginProfile", responseElements,
        AuthServerConfig.getReqId());
  }

  /**
   * This method will generate 'CreateAccountLoginProfile' response
   *
   * @param Account
   * @return LoginProfile XML of the requested account
 */

 public
  ServerResponse generateUpdateResponse() {
    return new XMLResponseFormatter().formatUpdateResponse(
        "UpdateAccountLoginProfile");
  }
}
