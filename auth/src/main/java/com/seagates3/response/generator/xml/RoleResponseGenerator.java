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
 * Original creation date: 1-Nov-2015
 */

package com.seagates3.response.generator.xml;

import com.seagates3.model.Role;
import com.seagates3.response.ServerResponse;
import io.netty.handler.codec.http.HttpResponseStatus;
import java.util.logging.Level;
import java.util.logging.Logger;
import javax.xml.parsers.ParserConfigurationException;
import javax.xml.transform.TransformerException;
import org.w3c.dom.Attr;
import org.w3c.dom.Document;
import org.w3c.dom.Element;

public class RoleResponseGenerator extends XMLResponseGenerator {
    public ServerResponse create(Role role) {
        Document doc;
        try {
            doc = xmlUtil.createNewDoc();
        } catch (ParserConfigurationException ex) {
            return null;
        }

        Element rootElement = doc.createElement("CreateRoleResponse");
        Attr attr = doc.createAttribute("xmlns");
        attr.setValue(IAM_XMLNS);
        rootElement.setAttributeNode(attr);
        doc.appendChild(rootElement);

        Element createRoleResponse = doc.createElement("CreateRoleResult");
        rootElement.appendChild(createRoleResponse);

        Element roleElement = doc.createElement("Role");
        createRoleResponse.appendChild(roleElement);

        Element pathEle = doc.createElement("Path");
        pathEle.appendChild(doc.createTextNode(role.getPath()));
        roleElement.appendChild(pathEle);

        String arnValue = String.format("arn:aws:iam::1:role/%s", role.getName());
        Element arn = doc.createElement("Arn");
        arn.appendChild(doc.createTextNode(arnValue));
        roleElement.appendChild(arn);

        Element roleName = doc.createElement("RoleName");
        roleName.appendChild(doc.createTextNode(role.getName()));
        roleElement.appendChild(roleName);

        Element policyDoc = doc.createElement("AssumeRolePolicyDocument");
        policyDoc.appendChild(doc.createTextNode(role.getRolePolicyDoc()));
        roleElement.appendChild(policyDoc);

        Element createDate = doc.createElement("CreateDate");
        createDate.appendChild(doc.createTextNode(role.getCreateDate()));
        roleElement.appendChild(createDate);

        /*
         * TODO
         * Implement ID for roles.
         */
        Element roleId = doc.createElement("RoleId");
        roleId.appendChild(doc.createTextNode(role.getName()));
        roleElement.appendChild(roleId);

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
        return success("DeleteRole");
    }

    public ServerResponse list(Role[] roleList) {
        Document doc;
        try {
            doc = xmlUtil.createNewDoc();
        } catch (ParserConfigurationException ex) {
            return null;
        }

        Element rootElement = doc.createElement("ListRolesResponse");
        Attr attr = doc.createAttribute("xmlns");
        attr.setValue(IAM_XMLNS);
        rootElement.setAttributeNode(attr);
        doc.appendChild(rootElement);

        Element listRolesResult = doc.createElement("ListRolesResult");
        rootElement.appendChild(listRolesResult);

        Element roles = doc.createElement("Roles");
        listRolesResult.appendChild(roles);

        for(Role role : roleList) {
            Element member = doc.createElement("member");
            roles.appendChild(member);

            Element path = doc.createElement("Path");
            path.appendChild(doc.createTextNode(role.getPath()));
            member.appendChild(path);

            String arn = "arn:aws:iam::000:roles/" + role.getName();
            Element arnEle = doc.createElement("Arn");
            arnEle.appendChild(doc.createTextNode(arn));
            member.appendChild(arnEle);

            Element roleName = doc.createElement("RoleName");
            roleName.appendChild(doc.createTextNode(role.getName()));
            member.appendChild(roleName);

            Element policyDoc = doc.createElement("AssumeRolePolicyDocument");
            policyDoc.appendChild(doc.createTextNode(role.getRolePolicyDoc()));
            member.appendChild(policyDoc);

            Element createDate = doc.createElement("CreateDate");
            createDate.appendChild(doc.createTextNode(role.getCreateDate()));
            member.appendChild(createDate);

            Element userId = doc.createElement("RoleId");
            userId.appendChild(doc.createTextNode(role.getName()));
            member.appendChild(userId);
        }

        Element isTruncated = doc.createElement("IsTruncated");
        isTruncated.appendChild(doc.createTextNode("false"));
        listRolesResult.appendChild(isTruncated);

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
}
