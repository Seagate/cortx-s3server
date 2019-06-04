/*
 * COPYRIGHT 2016 SEAGATE LLC
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
 * Original creation date: 27-May-2016
 */
package com.seagates3.response.formatter.xml;

import com.seagates3.response.ServerResponse;
import com.seagates3.util.BinaryUtil;
import io.netty.handler.codec.http.HttpResponseStatus;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.Map;
import javax.xml.parsers.ParserConfigurationException;
import javax.xml.transform.TransformerException;
import org.w3c.dom.Attr;
import org.w3c.dom.Document;
import org.w3c.dom.Element;

public class AuthorizationResponseFormatter extends XMLResponseFormatter {

    @Override
    public ServerResponse formatCreateResponse(String operation,
            String returnObject, LinkedHashMap<String, String> responseElements,
            String requestId) {
        throw new UnsupportedOperationException("Not supported yet.");
    }

    @Override
    public ServerResponse formatDeleteResponse(String operation) {
        throw new UnsupportedOperationException("Not supported yet.");
    }

    @Override
    public ServerResponse formatUpdateResponse(String operation) {
        throw new UnsupportedOperationException("Not supported yet.");
    }

    @Override
    public ServerResponse formatListResponse(String operation, String returnObject,
            ArrayList<LinkedHashMap<String, String>> responseElements,
            Boolean isTruncated, String requestId) {
        throw new UnsupportedOperationException("Not supported yet.");
    }

   public
    ServerResponse authorized(LinkedHashMap<String, String> responseElements,
                              String requestId, String acp) {

        Document doc;
        try {
            doc = createNewDoc();
        } catch (ParserConfigurationException ex) {
            return null;
        }

        Element responseElement = doc.createElement("AuthorizeUserResponse");
        Attr attr = doc.createAttribute("xmlns");
        attr.setValue(IAM_XMLNS);
        responseElement.setAttributeNode(attr);
        doc.appendChild(responseElement);

        Element resultElement = doc.createElement("AuthorizeUserResult");
        responseElement.appendChild(resultElement);

        for (Map.Entry<String, String> entry : responseElements.entrySet()) {
            Element element = doc.createElement(entry.getKey());
            element.appendChild(doc.createTextNode(entry.getValue()));
            resultElement.appendChild(element);
        }

        // Construct a default ACL and append as a child to resultElement
        if (acp != null)
          resultElement.appendChild(constructDefaultACLElement(doc, acp));

        Element responseMetadataElement = doc.createElement("ResponseMetadata");
        responseElement.appendChild(responseMetadataElement);

        Element requestIdElement = doc.createElement("RequestId");
        requestIdElement.appendChild(doc.createTextNode(requestId));
        responseMetadataElement.appendChild(requestIdElement);

        String responseBody;
        try {
            responseBody = docToString(doc);
            ServerResponse serverResponse = new ServerResponse(HttpResponseStatus.OK,
                    responseBody);

            return serverResponse;
        } catch (TransformerException ex) {
        }

        return null;
    }

    /**
     * Constructs an AccessControlPolicy (ACL) Element from acp -
     * XML, base64 encoded.
     * @param doc
     * @param acpXml
     * @return - the DefaultACL element
     */
   private
    Element constructDefaultACLElement(Document doc, String acpXml) {

      Element aclElement = null;

      if (acpXml != null) {
        aclElement = doc.createElement("ACL");
        aclElement.appendChild(
            doc.createTextNode(BinaryUtil.encodeToBase64String(acpXml)));
      }
      return aclElement;
    }
}
