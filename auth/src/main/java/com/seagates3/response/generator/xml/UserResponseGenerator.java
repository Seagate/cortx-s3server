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

import com.seagates3.model.User;
import com.seagates3.response.ServerResponse;

public class UserResponseGenerator extends XMLResponseGenerator {
    XMLUtil xmlUtil;

    public UserResponseGenerator() {
        xmlUtil = new XMLUtil();
    }

    public ServerResponse create(String name, String path, String userId) {
        Document doc;
        try {
            doc = xmlUtil.createNewDoc();
        } catch (ParserConfigurationException ex) {
            return null;
        }

        Element rootElement = doc.createElement("CreateUserResponse");
        Attr attr = doc.createAttribute("xmlns");
        attr.setValue(IAM_XMLNS);
        rootElement.setAttributeNode(attr);
        doc.appendChild(rootElement);

        Element createUserResponse = doc.createElement("CreateUserResult");
        rootElement.appendChild(createUserResponse);

        Element user = doc.createElement("User");
        createUserResponse.appendChild(user);

        Element pathEle = doc.createElement("Path");
        pathEle.appendChild(doc.createTextNode(path));
        user.appendChild(pathEle);

        Element userName = doc.createElement("UserName");
        userName.appendChild(doc.createTextNode(name));
        user.appendChild(userName);

        Element userIdEle = doc.createElement("UserId");
        userIdEle.appendChild(doc.createTextNode(userId));
        user.appendChild(userIdEle);

        String arnValue = String.format("arn:aws:iam::1:user/%s", name);
        Element arn = doc.createElement("Arn");
        arn.appendChild(doc.createTextNode(arnValue));
        user.appendChild(arn);

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

    public ServerResponse delete() {
        return success("DeleteUser");
    }

    public ServerResponse list(User[] userList) {

        Document doc;
        try {
            doc = xmlUtil.createNewDoc();
        } catch (ParserConfigurationException ex) {
            return null;
        }

        Element rootElement = doc.createElement("ListUsersResponse");
        Attr attr = doc.createAttribute("xmlns");
        attr.setValue(IAM_XMLNS);
        rootElement.setAttributeNode(attr);
        doc.appendChild(rootElement);

        Element listUserResult = doc.createElement("ListUsersResult");
        rootElement.appendChild(listUserResult);

        Element usersEle = doc.createElement("Users");
        listUserResult.appendChild(usersEle);

        for(User user : userList) {
            Element member = doc.createElement("Member");
            usersEle.appendChild(member);

            Element userId = doc.createElement("UserId");
            userId.appendChild(doc.createTextNode(user.getId()));
            member.appendChild(userId);

            Element path = doc.createElement("Path");
            path.appendChild(doc.createTextNode(user.getPath()));
            member.appendChild(path);

            Element userName = doc.createElement("UserName");
            userName.appendChild(doc.createTextNode(user.getName()));
            member.appendChild(userName);

            String arn = "arn:aws:iam::000:user/";
            Element arnEle = doc.createElement("Arn");
            arnEle.appendChild(doc.createTextNode(arn));
            member.appendChild(arnEle);

            Element createDate = doc.createElement("CreateDate");
            createDate.appendChild(doc.createTextNode(user.getCreateDate()));
            member.appendChild(createDate);

            Element passwordLastUsed = doc.createElement("PasswordLastUsed");
            passwordLastUsed.appendChild(doc.createTextNode(""));
            member.appendChild(passwordLastUsed);
        }

        Element isTruncated = doc.createElement("IsTruncated");
        isTruncated.appendChild(doc.createTextNode("false"));
        listUserResult.appendChild(isTruncated);

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
        return success("UpdateUser");
    }

    public ServerResponse authenticate() {
        Document doc;
        try {
            doc = xmlUtil.createNewDoc();
        } catch (ParserConfigurationException ex) {
            return null;
        }

        Element rootElement = doc.createElement("AuthenticateUserResponse");
        Attr attr = doc.createAttribute("xmlns");
        attr.setValue(IAM_XMLNS);
        rootElement.setAttributeNode(attr);
        doc.appendChild(rootElement);

        Element authenticatedUserResult = doc.createElement("AuthenticateUserResult");
        rootElement.appendChild(authenticatedUserResult);

        Element authenticated = doc.createElement("Authenticated");
        authenticated.appendChild(doc.createTextNode("True"));
        authenticatedUserResult.appendChild(authenticated);

        Element responseMetaData = doc.createElement("ResponseMetadata");
        rootElement.appendChild(responseMetaData);

        Element requestId = doc.createElement("RequestId");
        requestId.appendChild(doc.createTextNode("0000"));
        responseMetaData.appendChild(requestId);

        String responseBody;
        try {
            responseBody = xmlUtil.docToString(doc);
            ServerResponse serverResponse = new ServerResponse(HttpResponseStatus.ACCEPTED,
                                                responseBody);

            return serverResponse;
        } catch (TransformerException ex) {
        }

        return null;
    }
}
