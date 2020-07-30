package com.seagates3.response.generator;

import com.seagates3.authentication.ClientRequestToken;
import com.seagates3.authserver.AuthServerConfig;
import com.seagates3.model.Requestor;
import com.seagates3.response.ServerResponse;
import com.seagates3.response.formatter.xml.AuthenticationResponseFormatter;
import java.util.LinkedHashMap;
import io.netty.handler.codec.http.HttpResponseStatus;

public class AuthenticationResponseGenerator extends AbstractResponseGenerator {

    public ServerResponse generateAuthenticatedResponse(Requestor requestor,
            ClientRequestToken requestToken) {
        LinkedHashMap responseElements = new LinkedHashMap();
        responseElements.put("UserId", requestor.getId());
        responseElements.put("UserName", requestor.getName());
        responseElements.put("AccountId", requestor.getAccount().getId());
        responseElements.put("AccountName", requestor.getAccount().getName());
        responseElements.put("SignatureSHA256", requestToken.getSignature());
        responseElements.put("CanonicalId",
                             requestor.getAccount().getCanonicalId());
        responseElements.put("Email", requestor.getAccount().getEmail());

        return (ServerResponse) new AuthenticationResponseFormatter()
            .formatAuthenticatedResponse(responseElements,
                                         AuthServerConfig.getReqId());
    }

   public
    ServerResponse requestTimeTooSkewed(String requestTime, String serverTime) {
      String errorMessage =
          "The difference between request time and current time is too large";

      return (ServerResponse) new AuthenticationResponseFormatter()
          .formatSignatureErrorResponse(HttpResponseStatus.FORBIDDEN,
                                        "RequestTimeTooSkewed", errorMessage,
                                        requestTime, serverTime, "900000",
                                        AuthServerConfig.getReqId());
    }
}
