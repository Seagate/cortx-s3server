package com.seagates3.response.generator;

import com.seagates3.authserver.AuthServerConfig;
import com.seagates3.model.Requestor;
import com.seagates3.response.ServerResponse;
import com.seagates3.response.formatter.xml.AuthorizationResponseFormatter;
import java.util.LinkedHashMap;

public class AuthorizationResponseGenerator extends AbstractResponseGenerator {

  public
   ServerResponse generateAuthorizationResponse(Requestor requestor,
                                                String acpXml) {
        LinkedHashMap responseElements = new LinkedHashMap();
        if (requestor != null) {
          responseElements.put("UserId", requestor.getId());
          responseElements.put("UserName", requestor.getName());
          responseElements.put("AccountId", requestor.getAccount().getId());
          responseElements.put("AccountName", requestor.getAccount().getName());
          responseElements.put("CanonicalId",
                               requestor.getAccount().getCanonicalId());
        } else {
          responseElements.put("AllUserRequest", "true");
        }

        return (ServerResponse) new AuthorizationResponseFormatter().authorized(
            responseElements, AuthServerConfig.getReqId(), acpXml);
    }
}
