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

import java.util.logging.Level;
import java.util.logging.Logger;
import javax.xml.parsers.ParserConfigurationException;
import javax.xml.transform.TransformerException;

import io.netty.handler.codec.http.HttpResponseStatus;

import org.w3c.dom.Attr;
import org.w3c.dom.Document;
import org.w3c.dom.Element;

import com.seagates3.model.AccessKey;
import com.seagates3.response.ServerResponse;

public class AccountResponseGenerator extends XMLResponseGenerator {
    XMLUtil xmlUtil;

    public AccountResponseGenerator() {
        xmlUtil = new XMLUtil();
    }

    public ServerResponse create(AccessKey rootAccessKey) {

        Document doc;
        try {
            doc = xmlUtil.createNewDoc();
        } catch (ParserConfigurationException ex) {
            return null;
        }

        Element rootElement = doc.createElement("CreateAccountResponse");
        Attr attr = doc.createAttribute("xmlns");
        attr.setValue(IAM_XMLNS);
        rootElement.setAttributeNode(attr);
        doc.appendChild(rootElement);

        Element createAccountResponse = doc.createElement("CreateAccountResult");
        rootElement.appendChild(createAccountResponse);

        Element account = doc.createElement("Account");
        createAccountResponse.appendChild(account);

        Element rootUserName = doc.createElement("RootUserName");
        rootUserName.appendChild(doc.createTextNode("root"));
        account.appendChild(rootUserName);

        Element accessKeyId = doc.createElement("AccessKeyId");
        accessKeyId.appendChild(doc.createTextNode(rootAccessKey.getAccessKeyId()));
        account.appendChild(accessKeyId);

        Element statusEle = doc.createElement("Status");
        statusEle.appendChild(doc.createTextNode(rootAccessKey.getStatus()));
        account.appendChild(statusEle);

        Element secretAccessKey = doc.createElement("RootSecretKeyId");
        secretAccessKey.appendChild(doc.createTextNode(rootAccessKey.getSecretKey()));
        account.appendChild(secretAccessKey);

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
            Logger.getLogger(AccountResponseGenerator.class.getName()).log(Level.SEVERE, null, ex);
        }

        return null;
    }

    public ServerResponse entityAlreadyExists(String errorMessage) {
        return error(HttpResponseStatus.CONFLICT, "EntityAlreadyExists",
                errorMessage);
    }
}
