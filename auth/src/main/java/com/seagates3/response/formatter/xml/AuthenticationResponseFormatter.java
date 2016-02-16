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
 * Original creation date: 16-Dec-2015
 */

package com.seagates3.response.formatter.xml;

import io.netty.handler.codec.http.HttpResponseStatus;

import java.util.LinkedHashMap;
import java.util.Map;
import javax.xml.parsers.ParserConfigurationException;
import javax.xml.transform.TransformerException;

import org.w3c.dom.Attr;
import org.w3c.dom.Document;
import org.w3c.dom.Element;

import com.seagates3.response.ServerResponse;

public class AuthenticationResponseFormatter extends XMLResponseFormatter {

    public ServerResponse create(LinkedHashMap<String, String> responseElements,
            String requestId) {
        Document doc;
        try {
            doc = createNewDoc();
        } catch (ParserConfigurationException ex) {
            return null;
        }

        Element responseElement = doc.createElement("AuthenticateUserResponse");
        Attr attr = doc.createAttribute("xmlns");
        attr.setValue(IAM_XMLNS);
        responseElement.setAttributeNode(attr);
        doc.appendChild(responseElement);

        Element resultElement = doc.createElement("AuthenticateUserResult");
        responseElement.appendChild(resultElement);

        for (Map.Entry<String, String> entry : responseElements.entrySet()) {
            Element element = doc.createElement(entry.getKey());
            element.appendChild(doc.createTextNode(entry.getValue()));
            resultElement.appendChild(element);
        }

        Element responseMetadataElement = doc.createElement("ResponseMetadata");
        responseElement.appendChild(responseMetadataElement);

        Element requestIdElement = doc.createElement("RequestId");
        requestIdElement.appendChild(doc.createTextNode(requestId));
        responseMetadataElement.appendChild(requestIdElement);

        String responseBody;
        try {
            responseBody = docToString(doc);
            ServerResponse serverResponse = new ServerResponse(HttpResponseStatus.ACCEPTED,
                    responseBody);

            return serverResponse;
        } catch (TransformerException ex) {
        }

        return null;
    }
}
