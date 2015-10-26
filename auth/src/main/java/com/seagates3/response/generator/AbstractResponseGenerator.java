/*
 * COPYRIGHT 2015 SEAGATE LLC
 *
 * THIS DRAWING/DOCUMENT, ITS SPECIFICATIONS, AND THE DATA CONTAINED
 * HEREIN, ARE THE EXCLUSIVE PROPERTY OF SEAGATE TECHNOLOGY
 * LIMITED, ISSUED IN STRICT CONFIDENCE AND SHALL NOT, WITHOUT
 * THE PRIOR WRITTEN PERMISSION OF SEAGATE TECHNOLOGY LIMITED,
 * BE REPRODUCED, COPIED, OR DISCLOSED TO A THIRD PARTY, OR
 * USED FOR ANY PURPOSE WHATSOEVER, OR STORED IN A RETRIEVAL SYSTEM
 * EXCEPT AS ALLOWED BY THE TERMS OF SEAGATE LICENSES AND AGREEMENTS.
 *
 * YOU SHOULD HAVE RECEIVED A COPY OF SEAGATE'S LICENSE ALONG WITH
 * THIS RELEASE. IF NOT PLEASE CONTACT A SEAGATE REPRESENTATIVE
 * http://www.seagate.com/contact
 *
 * Original author:  Arjun Hariharan <arjun.hariharan@seagate.com>
 * Original creation date: 17-Sep-2014
 */

package com.seagates3.response.generator;

import io.netty.handler.codec.http.HttpResponseStatus;

import com.seagates3.response.ServerResponse;

public abstract class AbstractResponseGenerator {
    protected final String IAM_XMLNS = "https://iam.seagate.com/doc/2010-05-08/";

    public ServerResponse badRequest() {
        String errorMessage = "Bad Request. Check request headers and body.";

        return error(HttpResponseStatus.BAD_REQUEST,
                "BadRequest", errorMessage);
    }

    public ServerResponse deleteConflict() {
        String errorMessage = "The request was rejected because it attempted "
                + "to delete a resource that has attached subordinate entities. "
                + "The error message describes these entities.";

        return error(HttpResponseStatus.CONFLICT, "DeleteConflict", errorMessage);
    }

    public ServerResponse entityAlreadyExists() {
        String errorMessage = "The request was rejected because it attempted "
                + "to create or update a resource that already exists.";

        return error(HttpResponseStatus.CONFLICT,
                "EntityAlreadyExists", errorMessage);
    }

    public ServerResponse expiredCredential() {
        String errorMessage = "The request was rejected because the credential"
                + "used to sign the request has expired.";

        return error(HttpResponseStatus.FORBIDDEN,
                "ExpiredCredential", errorMessage);
    }

    public ServerResponse inactiveAccessKey() {
        String errorMessage = "The access key used to sign the request is inactive.";
        return error(HttpResponseStatus.FORBIDDEN, "InactiveAccessKey",
                errorMessage);
    }

    public ServerResponse incorrectSignature() {
        String errorMessage = "The request signature we calculated does not "
                        + "match the signature you provided. Check your AWS "
                        + "Secret Access Key and signing method.";
        return error(HttpResponseStatus.FORBIDDEN, "IncorrectSignature",
                errorMessage);
    }

    public ServerResponse internalServerError() {
        String errorMessage = "The request processing has failed because of an "
                    + "unknown error, exception or failure.";

        return error(HttpResponseStatus.INTERNAL_SERVER_ERROR,
                "InternalFailure", errorMessage);
    }

    public ServerResponse invalidAction() {
        String errorMessage = "The action or operation requested is "
                + "invalid. Verify that the action is typed correctly.";

        return error(HttpResponseStatus.BAD_REQUEST, "InvalidAction",
                errorMessage);
    }

    public ServerResponse invalidClientTokenId() {
        String errorMessage = "The X.509 certificate or AWS access key ID "
                + "provided does not exist in our records.";

        return error(HttpResponseStatus.FORBIDDEN, "InvalidClientTokenId",
                errorMessage);
    }

    public ServerResponse invalidParametervalue() {
        String errorMessage = "An invalid or out-of-range value was "
                    + "supplied for the input parameter.";

        return error(HttpResponseStatus.BAD_REQUEST,
                "InvalidParameterValue", errorMessage);
    }

    public ServerResponse missingParameter() {
        String errorMessage = "A required parameter for the specified action "
                + "is not supplied.";

        return error(HttpResponseStatus.BAD_REQUEST, "MissingParameter",
                errorMessage);
    }

    public ServerResponse noSuchEntity() {
        String errorMessage = "The request was rejected because it referenced an "
                + "entity that does not exist. ";

        return error(HttpResponseStatus.NOT_FOUND, "NoSuchEntity", errorMessage);
    }

    /*
     * Use this method for internal purpose.
     */
    public ServerResponse ok() {
        String errorMessage = "Action successful";
        return new ServerResponse(HttpResponseStatus.OK, errorMessage);
    }

    /*
     * Provide implementation in the sub class.
     */
    public abstract ServerResponse success(String action);

    /*
     * Provide implementation in the sub class.
     */
    public abstract ServerResponse error(HttpResponseStatus httpResponseStatus,
            String code, String message);
}
