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

package com.seagates3.response.generator.xml;

import javax.xml.parsers.ParserConfigurationException;
import javax.xml.transform.TransformerException;

import io.netty.handler.codec.http.HttpResponseStatus;

import org.w3c.dom.Attr;
import org.w3c.dom.Document;
import org.w3c.dom.Element;

import com.seagates3.model.AccessKey;
import com.seagates3.model.User;
import com.seagates3.response.ServerResponse;
import com.seagates3.util.DateUtil;

public class FederationTokenResponseGenerator extends XMLResponseGenerator {
    XMLUtil xmlUtil;

    public FederationTokenResponseGenerator() {
        xmlUtil = new XMLUtil();
    }

    public ServerResponse create(User user, AccessKey accessKey) {

        Document doc;
        try {
            doc = xmlUtil.createNewDoc();
        } catch (ParserConfigurationException ex) {
            return null;
        }

        Element rootElement = doc.createElement("GetFederationTokenResponse");
        Attr attr = doc.createAttribute("xmlns");
        attr.setValue(IAM_XMLNS);
        rootElement.setAttributeNode(attr);
        doc.appendChild(rootElement);

        Element getFederationTokenResult = doc.createElement("GetFederationTokenResult");
        rootElement.appendChild(getFederationTokenResult);

        Element credentials = doc.createElement("Credentials");
        getFederationTokenResult.appendChild(credentials);

        Element sessionToken = doc.createElement("SessionToken");
        sessionToken.appendChild(doc.createTextNode(accessKey.getToken()));
        credentials.appendChild(sessionToken);

        Element secretAccessKey = doc.createElement("SecretAccessKey");
        secretAccessKey.appendChild(doc.createTextNode(accessKey.getSecretKey()));
        credentials.appendChild(secretAccessKey);

        String expirationDate = DateUtil.toServerResponseFormat(accessKey.getExpiry());
        Element expiryDate = doc.createElement("Expiration");
        expiryDate.appendChild(doc.createTextNode(expirationDate));
        credentials.appendChild(expiryDate);

        Element accessKeyIdEle = doc.createElement("AccessKeyId");
        accessKeyIdEle.appendChild(doc.createTextNode(accessKey.getAccessKeyId()));
        credentials.appendChild(accessKeyIdEle);

        Element federatedUser = doc.createElement("FederatedUser");
        getFederationTokenResult.appendChild(federatedUser);

        String arnValue = String.format("arn:aws:sts::1:federated-user/%s", user.getName());
        Element arn = doc.createElement("Arn");
        arn.appendChild(doc.createTextNode(arnValue));
        federatedUser.appendChild(arn);

        String fedUseIdVal = String.format("%s:%s",user.getId(), user.getName());
        Element federatedUserId = doc.createElement("FederatedUserId");
        federatedUserId.appendChild(doc.createTextNode(fedUseIdVal));
        federatedUser.appendChild(federatedUserId);

        Element packedPolicySize = doc.createElement("PackedPolicySize");
        packedPolicySize.appendChild(doc.createTextNode("6"));
        getFederationTokenResult.appendChild(packedPolicySize);


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
}
