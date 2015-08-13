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
import java.util.logging.Level;
import java.util.logging.Logger;
import javax.xml.parsers.ParserConfigurationException;
import javax.xml.transform.TransformerException;

import org.w3c.dom.Attr;
import org.w3c.dom.Document;
import org.w3c.dom.Element;

import com.seagates3.response.ServerResponse;
import com.seagates3.response.generator.AbstractResponseGenerator;

public class XMLResponseGenerator extends AbstractResponseGenerator {

    @Override
    public ServerResponse success(String action) {
        XMLUtil domHelper = new XMLUtil();
        Document doc;
        try {
            doc = domHelper.createNewDoc();
        } catch (ParserConfigurationException ex) {
            return null;
        }

        Element rootElement = doc.createElement(action + "Response");
        Attr attr = doc.createAttribute("xmlns");
        attr.setValue(IAM_XMLNS);
        rootElement.setAttributeNode(attr);
        doc.appendChild(rootElement);

        Element responseMetaData = doc.createElement("ResponseMetadata");
        rootElement.appendChild(responseMetaData);

        Element requestId = doc.createElement("RequestId");
        requestId.appendChild(doc.createTextNode("0000"));
        responseMetaData.appendChild(requestId);

        String responseBody;
        try {
            responseBody = domHelper.docToString(doc);
            ServerResponse serverResponse = new ServerResponse(HttpResponseStatus.OK,
                                                responseBody);

            return serverResponse;
        } catch (TransformerException ex) {
        }

        return null;
    }

    @Override
    public ServerResponse error(HttpResponseStatus httpResponseStatus,
            String code, String message) {
        XMLUtil domHelper = new XMLUtil();

        Document doc;
        try {
            doc = domHelper.createNewDoc();
        } catch (ParserConfigurationException ex) {
            return null;
        }

        Element rootElement = doc.createElement("Error");
        Attr attr = doc.createAttribute("xmlns");
        attr.setValue(IAM_XMLNS);
        rootElement.setAttributeNode(attr);
        doc.appendChild(rootElement);

        Element codeEle = doc.createElement("Code");
        codeEle.appendChild(doc.createTextNode(code));
        rootElement.appendChild(codeEle);

        Element messageEle = doc.createElement("Message");
        messageEle.appendChild(doc.createTextNode(message));
        rootElement.appendChild(messageEle);

        Element requestIdEle = doc.createElement("RequestId");
        requestIdEle.appendChild(doc.createTextNode("0000"));
        rootElement.appendChild(requestIdEle);

        String responseBody;
        try {
            responseBody = domHelper.docToString(doc);
            ServerResponse serverResponse = new ServerResponse(httpResponseStatus,
                                                responseBody);

            return serverResponse;
        } catch (TransformerException ex) {
            Logger.getLogger(AccountResponseGenerator.class.getName()).log(Level.SEVERE, null, ex);
        }

        ServerResponse serverResponse = new ServerResponse();
        return serverResponse;
    }
}
