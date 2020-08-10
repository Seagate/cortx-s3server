/*
 * Copyright (c) 2020 Seagate Technology LLC and/or its Affiliates
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * For any questions about this software or licensing,
 * please email opensource@seagate.com or cortx-questions@seagate.com.
 *
 */

package com.seagates3.response.formatter.xml;

import com.seagates3.response.ServerResponse;
import io.netty.handler.codec.http.HttpResponseStatus;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.Map;
import javax.xml.parsers.ParserConfigurationException;
import javax.xml.transform.TransformerException;
import org.w3c.dom.Attr;
import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class AuthenticationResponseFormatter extends XMLResponseFormatter {

  private
   final Logger LOGGER =
       LoggerFactory.getLogger(AuthenticationResponseFormatter.class.getName());
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

    public ServerResponse formatAuthenticatedResponse(
            LinkedHashMap<String, String> responseElements, String requestId) {
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
            ServerResponse serverResponse = new ServerResponse(HttpResponseStatus.OK,
                    responseBody);

            return serverResponse;
        } catch (TransformerException ex) {
        }

        return null;
    }

   public
    ServerResponse formatSignatureErrorResponse(
        HttpResponseStatus httpResponseStatus, String code, String message,
        String requestTime, String serverTime,
        String maxAllowedSkewMilliseconds, String requestId) {
      Document doc;
      try {
        doc = createNewDoc();
      }
      catch (ParserConfigurationException ex) {
        return null;
      }

      Element root = doc.createElement("ErrorResponse");
      Attr attr = doc.createAttribute("xmlns");
      attr.setValue(IAM_XMLNS);
      root.setAttributeNode(attr);
      doc.appendChild(root);

      Element errorEle = doc.createElement("Error");
      root.appendChild(errorEle);

      Element codeEle = doc.createElement("Code");
      codeEle.appendChild(doc.createTextNode(code));
      errorEle.appendChild(codeEle);

      Element messageEle = doc.createElement("Message");
      messageEle.appendChild(doc.createTextNode(message));
      errorEle.appendChild(messageEle);

      Element requestTimeEle = doc.createElement("RequestTime");
      requestTimeEle.appendChild(doc.createTextNode(requestTime));
      errorEle.appendChild(requestTimeEle);

      Element serverTimeEle = doc.createElement("ServerTime");
      serverTimeEle.appendChild(doc.createTextNode(serverTime));
      errorEle.appendChild(serverTimeEle);

      Element maxAllowedSkewEle =
          doc.createElement("MaxAllowedSkewMilliseconds");
      maxAllowedSkewEle.appendChild(
          doc.createTextNode(maxAllowedSkewMilliseconds));
      errorEle.appendChild(maxAllowedSkewEle);

      Element requestIdEle = doc.createElement("RequestId");
      requestIdEle.appendChild(doc.createTextNode(requestId));
      errorEle.appendChild(requestIdEle);

      String responseBody;
      try {
        responseBody = docToString(doc);
        LOGGER.debug("AuthenticationResponseFormatter :: " +
                     "formatSignatureErrorResponse()" + "- responseBody is - " +
                     responseBody);
        ServerResponse serverResponse =
            new ServerResponse(httpResponseStatus, responseBody);

        return serverResponse;
      }
      catch (TransformerException ex) {
      }

      return null;
    }
}
