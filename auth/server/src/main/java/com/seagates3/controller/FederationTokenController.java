package com.seagates3.controller;

import com.seagates3.exception.DataAccessException;
import com.seagates3.model.AccessKey;
import com.seagates3.model.Requestor;
import com.seagates3.model.User;
import com.seagates3.response.ServerResponse;
import com.seagates3.response.generator.FederationTokenResponseGenerator;
import com.seagates3.service.AccessKeyService;
import com.seagates3.service.UserService;
import java.util.Map;

public class FederationTokenController extends AbstractController {

    FederationTokenResponseGenerator fedTokenResponse;

    public FederationTokenController(Requestor requestor,
            Map<String, String> requestBody) {
        super(requestor, requestBody);

        fedTokenResponse = new FederationTokenResponseGenerator();
    }

    /*
     * TODO
     * Store user policy
     */
    @Override
    public ServerResponse create() throws DataAccessException {
        String userName = requestBody.get("Name");

        int duration;
        if (requestBody.containsKey("DurationSeconds")) {
            duration = Integer.parseInt(requestBody.get("DurationSeconds"));
        } else {
            duration = 43200;
        }

        User user = UserService.createFederationUser(requestor.getAccount(),
                userName);

        AccessKey accessKey = AccessKeyService.createFedAccessKey(user,
                duration);
        if (accessKey == null) {
            return fedTokenResponse.internalServerError();
        }

        return fedTokenResponse.generateCreateResponse(user, accessKey);
    }
}
