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
 * Original creation date: 2-Nov-2015
 */

package com.seagates3.response.generator.xml;

import com.seagates3.model.AccessKey;
import com.seagates3.model.SAMLResponse;
import com.seagates3.model.User;
import com.seagates3.response.ServerResponse;
import io.netty.handler.codec.http.HttpResponseStatus;
import javax.xml.parsers.ParserConfigurationException;
import javax.xml.transform.TransformerException;
import org.w3c.dom.Attr;
import org.w3c.dom.Document;
import org.w3c.dom.Element;

public class AssumeRoleWithSAMLResponseGenerator extends XMLResponseGenerator {
    public ServerResponse create(User user,SAMLResponse samlResponse,
            AccessKey accessKey) {
        Document doc;
        try {
            doc = xmlUtil.createNewDoc();
        } catch (ParserConfigurationException ex) {
            return null;
        }

        Element rootElement = doc.createElement("AssumeRoleWithSAMLResponse");
        Attr attr = doc.createAttribute("xmlns");
        attr.setValue(IAM_XMLNS);
        rootElement.setAttributeNode(attr);
        doc.appendChild(rootElement);

        Element result = doc.createElement("AssumeRoleWithSAMLResult");
        rootElement.appendChild(result);

        Element credentials = doc.createElement("Credentials");
        result.appendChild(credentials);

        Element sessionToken = doc.createElement("SessionToken");
        sessionToken.appendChild(doc.createTextNode(accessKey.getToken()));
        credentials.appendChild(sessionToken);

        Element secretAccessKey = doc.createElement("SecretAccessKey");
        secretAccessKey.appendChild(doc.createTextNode(accessKey.getSecretKey()));
        credentials.appendChild(secretAccessKey);

        Element expiryDate = doc.createElement("Expiration");
        expiryDate.appendChild(doc.createTextNode(accessKey.getExpiry()));
        credentials.appendChild(expiryDate);

        Element accessKeyIdEle = doc.createElement("AccessKeyId");
        accessKeyIdEle.appendChild(doc.createTextNode(accessKey.getAccessKeyId()));
        credentials.appendChild(accessKeyIdEle);

        Element assumedRoleUser = doc.createElement("AssumedRoleUser");
        result.appendChild(assumedRoleUser);

        String arnValue = String.format("arn:aws:sts::1:assumed-role/%s",
                user.getRoleName());
        Element arn = doc.createElement("Arn");
        arn.appendChild(doc.createTextNode(arnValue));
        assumedRoleUser.appendChild(arn);

        String assumedRoleIdVal = String.format("%s:%s",user.getId(),
                samlResponse.getRoleSessionName());
        Element assumedRoleId = doc.createElement("AssumedRoleId");
        assumedRoleId.appendChild(doc.createTextNode(assumedRoleIdVal));
        assumedRoleUser.appendChild(assumedRoleId);

        Element issuer = doc.createElement("Issuer");
        issuer.appendChild(doc.createTextNode(samlResponse.getIssuer()));
        result.appendChild(issuer);

        Element audience = doc.createElement("Audience");
        audience.appendChild(doc.createTextNode(samlResponse.getAudience()));
        result.appendChild(audience);

        Element subject = doc.createElement("Subject");
        subject.appendChild(doc.createTextNode(samlResponse.getsubject()));
        result.appendChild(subject);

        Element subjectType = doc.createElement("SubjectType");
        subjectType.appendChild(doc.createTextNode(samlResponse.getsubjectType()));
        result.appendChild(subjectType);

        /*
         * TODO
         * Calculate Name Qualifier.
         */
        Element nameQualifier = doc.createElement("NameQualifier");
        nameQualifier.appendChild(doc.createTextNode("S4jYAZtpWsPHGsy1j5sKtEKOyW4="));
        result.appendChild(nameQualifier);

        Element packedPolicySize = doc.createElement("PackedPolicySize");
        packedPolicySize.appendChild(doc.createTextNode("6"));
        result.appendChild(packedPolicySize);


        Element responseMetaData = doc.createElement("ResponseMetadata");
        rootElement.appendChild(responseMetaData);

        Element requestId = doc.createElement("RequestId");
        requestId.appendChild(doc.createTextNode("0000"));
        responseMetaData.appendChild(requestId);

        String responseBody;
        try {
            responseBody = xmlUtil.docToString(doc);
            ServerResponse serverResponse = new ServerResponse(HttpResponseStatus.CREATED,
                                                responseBody);

            return serverResponse;
        } catch (TransformerException ex) {
        }

        return null;
    }

    public ServerResponse invalidIdentityToken() {
        String errorMessage = "The web identity token that was passed could "
                + "not be validated. Get a new identity token from the "
                + "identity provider and then retry the request.";

        return error(HttpResponseStatus.BAD_REQUEST, "InvalidIdentityToken",
                errorMessage);
    }

    public ServerResponse expiredToken() {
        String errorMessage = "The web identity token that was passed is "
                + "expired or is not valid. Get a new identity token from the "
                + "identity provider and then retry the request.";

        return error(HttpResponseStatus.BAD_REQUEST, "ExpiredToken",
                errorMessage);
    }

    public ServerResponse idpRejectedClaim() {
        String errorMessage = "The identity provider (IdP) reported that "
                + "authentication failed. This might be because the claim is "
                + "invalid.\nIf this error is returned for the "
                + "AssumeRoleWithWebIdentity operation, it can also mean that "
                + "the claim has expired or has been explicitly revoked.";

        return error(HttpResponseStatus.FORBIDDEN, "IDPRejectedClaim",
                errorMessage);
    }

}
