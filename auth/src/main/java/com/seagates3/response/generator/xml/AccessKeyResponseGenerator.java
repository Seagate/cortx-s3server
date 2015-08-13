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


import io.netty.handler.codec.http.HttpResponseStatus;

import javax.xml.parsers.ParserConfigurationException;
import javax.xml.transform.TransformerException;

import org.w3c.dom.Attr;
import org.w3c.dom.Document;
import org.w3c.dom.Element;

import com.seagates3.model.AccessKey;
import com.seagates3.response.ServerResponse;

public class AccessKeyResponseGenerator extends XMLResponseGenerator {
    XMLUtil xmlUtil;

    public AccessKeyResponseGenerator() {
        xmlUtil = new XMLUtil();
    }

    public ServerResponse create(String userName, AccessKey accessKey) {

        Document doc;
        try {
            doc = xmlUtil.createNewDoc();
        } catch (ParserConfigurationException ex) {
            return null;
        }

        Element rootElement = doc.createElement("CreateAccessKeyResponse");
        Attr attr = doc.createAttribute("xmlns");
        attr.setValue(IAM_XMLNS);
        rootElement.setAttributeNode(attr);
        doc.appendChild(rootElement);

        Element createAccessKeyResponse = doc.createElement("CreateAccessKeyResult");
        rootElement.appendChild(createAccessKeyResponse);

        Element accessKeyEle = doc.createElement("AccessKey");
        createAccessKeyResponse.appendChild(accessKeyEle);

        Element userNameEle = doc.createElement("UserName");
        userNameEle.appendChild(doc.createTextNode(userName));
        accessKeyEle.appendChild(userNameEle);

        Element accessKeyIdEle = doc.createElement("AccessKeyId");
        accessKeyIdEle.appendChild(doc.createTextNode(accessKey.getAccessKeyId()));
        accessKeyEle.appendChild(accessKeyIdEle);

        Element statusEle = doc.createElement("Status");
        statusEle.appendChild(doc.createTextNode(accessKey.getStatus()));
        accessKeyEle.appendChild(statusEle);

        Element secretAccessKey = doc.createElement("SecretAccessKey");
        secretAccessKey.appendChild(doc.createTextNode(accessKey.getSecretKey()));
        accessKeyEle.appendChild(secretAccessKey);

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

    public ServerResponse delete() {
        return success("DeleteAccessKey");
    }

    public ServerResponse list(String userName, AccessKey[] accessKeyList) {

        Document doc;
        try {
            doc = xmlUtil.createNewDoc();
        } catch (ParserConfigurationException ex) {
            return null;
        }

        Element rootElement = doc.createElement("ListAccessKeysResponse");
        Attr attr = doc.createAttribute("xmlns");
        attr.setValue(IAM_XMLNS);
        rootElement.setAttributeNode(attr);
        doc.appendChild(rootElement);

        Element listAccessKeyResult = doc.createElement("ListAccessKeysResult");
        rootElement.appendChild(listAccessKeyResult);

        Element userNameEle = doc.createElement("UserName");
        userNameEle.appendChild(doc.createTextNode(userName));
        listAccessKeyResult.appendChild(userNameEle);

        Element accessKeyMetadataEle = doc.createElement("AccessKeyMetadata");
        listAccessKeyResult.appendChild(accessKeyMetadataEle);

        for(AccessKey accessKey : accessKeyList) {
            Element member = doc.createElement("Member");
            accessKeyMetadataEle.appendChild(member);

            userNameEle = doc.createElement("UserName");
            userNameEle.appendChild(doc.createTextNode(userName));
            member.appendChild(userNameEle);

            Element accessKeyId = doc.createElement("AccessKeyId");
            accessKeyId.appendChild(doc.createTextNode(accessKey.getAccessKeyId()));
            member.appendChild(accessKeyId);

            Element status = doc.createElement("Status");
            status.appendChild(doc.createTextNode(accessKey.getStatus()));
            member.appendChild(status);

            Element createDate = doc.createElement("CreateDate");
            createDate.appendChild(doc.createTextNode(accessKey.getCreateDate()));
            member.appendChild(createDate);
        }

        Element isTruncated = doc.createElement("IsTruncated");
        isTruncated.appendChild(doc.createTextNode("false"));
        listAccessKeyResult.appendChild(isTruncated);

        Element responseMetaData = doc.createElement("ResponseMetadata");
        rootElement.appendChild(responseMetaData);

        Element requestId = doc.createElement("RequestId");
        requestId.appendChild(doc.createTextNode("0000"));
        responseMetaData.appendChild(requestId);

        String responseBody;
        try {
            responseBody = xmlUtil.docToString(doc);
            ServerResponse serverResponse = new ServerResponse(HttpResponseStatus.OK,
                                                responseBody);

            return serverResponse;
        } catch (TransformerException ex) {
        }

        return null;
    }

    public ServerResponse update() {
        return success("UpdateAccessKey");
    }

    public ServerResponse accessKeyQuotaExceeded() {
        String errorMessage = "Cannot exceed quota for AccessKeysPerUser: 2";
        return error(HttpResponseStatus.CONFLICT, "AccessKeyQuoteExceeded",
                errorMessage);
    }

    public ServerResponse noSuchEntity(String errorMessage) {
        return error(HttpResponseStatus.CONFLICT, "NoSuchEntity",
                errorMessage);
    }
}
