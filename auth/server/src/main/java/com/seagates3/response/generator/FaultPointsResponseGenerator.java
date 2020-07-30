package com.seagates3.response.generator;

import com.seagates3.response.ServerResponse;
import io.netty.handler.codec.http.HttpResponseStatus;

public class FaultPointsResponseGenerator extends AbstractResponseGenerator {

    public ServerResponse badRequest(String errorMessage) {
        return formatResponse(HttpResponseStatus.BAD_REQUEST,
                "BadRequest", errorMessage);
    }

    public ServerResponse invalidParametervalue(String errorMessage) {
        return formatResponse(HttpResponseStatus.BAD_REQUEST,
                "InvalidParameterValue", errorMessage);
    }

    public ServerResponse faultPointAlreadySet(String errorMessage) {

        return formatResponse(HttpResponseStatus.CONFLICT,
                "FaultPointAlreadySet", errorMessage);
    }

    public ServerResponse faultPointNotSet(String errorMessage) {

        return formatResponse(HttpResponseStatus.CONFLICT,
                "FaultPointNotSet", errorMessage);
    }

    public ServerResponse setSuccessful() {
        return new ServerResponse(HttpResponseStatus.CREATED,
                "Fault point set successfully.");
    }

    public ServerResponse resetSuccessful() {
        return new ServerResponse(HttpResponseStatus.OK,
                "Fault point deleted successfully.");
    }
}
