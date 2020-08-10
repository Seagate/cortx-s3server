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

import java.util.LinkedHashMap;
import java.util.Map;

import javax.xml.parsers.ParserConfigurationException;
import javax.xml.transform.TransformerException;

import org.w3c.dom.Attr;
import org.w3c.dom.Document;
import org.w3c.dom.Element;

import com.seagates3.response.ServerResponse;

import io.netty.handler.codec.http.HttpResponseStatus;

public class SAMLSessionResponseFormatter extends XMLResponseFormatter {

    @Override
    public ServerResponse formatCreateResponse(String operation, String returnObject,
            LinkedHashMap<String, String> responseElements, String requestId) {
      throw new UnsupportedOperationException("create Not supported yet.");
    }

    public ServerResponse formatCreateReponse(
            LinkedHashMap<String, String> credentials,
            LinkedHashMap<String, String> userDetails,
            String requestId) {

       Document saml_session_doc;
        try {
          saml_session_doc = createNewDoc();
        } catch (ParserConfigurationException ex) {
            return null;
        }

        Element samlResponseElement =
            saml_session_doc.createElement("SessionTokenResponse");
        Attr attr = saml_session_doc.createAttribute("xmlns");
        attr.setValue(IAM_XMLNS);
        samlResponseElement.setAttributeNode(attr);
        saml_session_doc.appendChild(samlResponseElement);

        Element samlResultElement =
            saml_session_doc.createElement("SessionTokenResult");
        samlResponseElement.appendChild(samlResultElement);

        Element credElement = saml_session_doc.createElement("Credentials");
        samlResultElement.appendChild(credElement);
        for (Map.Entry<String, String> credentialEntry :
             credentials.entrySet()) {
          Element element =
              saml_session_doc.createElement(credentialEntry.getKey());
          element.appendChild(
              saml_session_doc.createTextNode(credentialEntry.getValue()));
            credElement.appendChild(element);
        }

        Element userElement = saml_session_doc.createElement("User");
        samlResultElement.appendChild(userElement);
        for (Map.Entry<String, String> entry : userDetails.entrySet()) {
          Element element = saml_session_doc.createElement(entry.getKey());
          element.appendChild(
              saml_session_doc.createTextNode(entry.getValue()));
            userElement.appendChild(element);
        }

        Element samlResponseMetadataElement =
            saml_session_doc.createElement("ResponseMetadata");
        samlResponseElement.appendChild(samlResponseMetadataElement);

        Element samlRequestIdElement =
            saml_session_doc.createElement("RequestId");
        samlRequestIdElement.appendChild(
            saml_session_doc.createTextNode(requestId));
        samlResponseMetadataElement.appendChild(samlRequestIdElement);

        String responseBody;
        try {
          responseBody = docToString(saml_session_doc);
          ServerResponse saml_session_server_response =
              new ServerResponse(HttpResponseStatus.OK, responseBody);

          return saml_session_server_response;
        } catch (TransformerException ex) {
        }

        return null;
    }
}

