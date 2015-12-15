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
 * Original creation date: [Date]
 */

package com.seagates3.response.generator.xml;

import com.seagates3.model.SAMLProvider;
import com.seagates3.response.ServerResponse;
import io.netty.handler.codec.http.HttpResponseStatus;
import java.util.logging.Level;
import java.util.logging.Logger;
import javax.xml.parsers.ParserConfigurationException;
import javax.xml.transform.TransformerException;
import org.w3c.dom.Attr;
import org.w3c.dom.Document;
import org.w3c.dom.Element;

public class SAMLProviderResponseGenerator extends XMLResponseGenerator {
    public ServerResponse create(String name) {
        Document doc;
        try {
            doc = xmlUtil.createNewDoc();
        } catch (ParserConfigurationException ex) {
            return null;
        }

        Element rootElement = doc.createElement("CreateSAMLProviderResponse");
        Attr attr = doc.createAttribute("xmlns");
        attr.setValue(IAM_XMLNS);
        rootElement.setAttributeNode(attr);
        doc.appendChild(rootElement);

        Element createUserResponse = doc.createElement("CreateSAMLProviderResult");
        rootElement.appendChild(createUserResponse);

        String arnValue = String.format("arn:aws:iam::1:saml-metadata/%s", name);
        Element arn = doc.createElement("SAMLProviderArn");
        arn.appendChild(doc.createTextNode(arnValue));
        createUserResponse.appendChild(arn);

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
        return success("DeleteSAMLProvider");
    }

    public ServerResponse update(String name) {
        Document doc;
        try {
            doc = xmlUtil.createNewDoc();
        } catch (ParserConfigurationException ex) {
            return null;
        }

        Element rootElement = doc.createElement("UpdateSAMLProviderResponse");
        Attr attr = doc.createAttribute("xmlns");
        attr.setValue(IAM_XMLNS);
        rootElement.setAttributeNode(attr);
        doc.appendChild(rootElement);

        Element updateUserResponse = doc.createElement("UpdateSAMLProviderResult");
        rootElement.appendChild(updateUserResponse);

        String arnValue = String.format("arn:aws:iam::1:saml-metadata/%s", name);
        Element arn = doc.createElement("SAMLProviderArn");
        arn.appendChild(doc.createTextNode(arnValue));
        updateUserResponse.appendChild(arn);

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

    public ServerResponse list(SAMLProvider[] samlProviderList) {
        Document doc;
        try {
            doc = xmlUtil.createNewDoc();
        } catch (ParserConfigurationException ex) {
            return null;
        }

        Element rootElement = doc.createElement("ListSAMLProvidersResponse");
        Attr attr = doc.createAttribute("xmlns");
        attr.setValue(IAM_XMLNS);
        rootElement.setAttributeNode(attr);
        doc.appendChild(rootElement);

        Element result = doc.createElement("ListSAMLProvidersResult");
        rootElement.appendChild(result);

        Element samlProviderListEle = doc.createElement("SAMLProviderList");
        result.appendChild(samlProviderListEle);

        for(SAMLProvider samlProvider : samlProviderList) {
            Element member = doc.createElement("member");
            samlProviderListEle.appendChild(member);

            String arn = String.format("arn:aws:iam::000:instance-profile/%s",
                    samlProvider.getName());
            Element arnEle = doc.createElement("Arn");
            arnEle.appendChild(doc.createTextNode(arn));
            member.appendChild(arnEle);

            Element validUntil = doc.createElement("ValidUntil");
            validUntil.appendChild(doc.createTextNode(samlProvider.getExpiry()));
            member.appendChild(validUntil);

            Element createDate = doc.createElement("CreateDate");
            createDate.appendChild(doc.createTextNode(samlProvider.getCreateDate()));
            member.appendChild(createDate);
        }

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
