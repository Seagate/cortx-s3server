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
import java.util.LinkedHashMap;
import java.util.Map;
import javax.xml.parsers.ParserConfigurationException;
import javax.xml.transform.TransformerException;
import org.w3c.dom.Attr;
import org.w3c.dom.Document;
import org.w3c.dom.Element;

public class FederationTokenResponseFormatter extends XMLResponseFormatter {

    @Override
    public ServerResponse formatCreateResponse(String operation, String returnObject,
            LinkedHashMap<String, String> responseElements, String requestId) {
        throw new UnsupportedOperationException("Not supported yet.");
    }

    public ServerResponse formatCreateResponse(LinkedHashMap<String, String> credentials,
            LinkedHashMap<String, String> federatedUser, String packedPolicy,
            String requestId) {
        Document doc;
        try {
            doc = createNewDoc();
        } catch (ParserConfigurationException ex) {
            return null;
        }

        Element responseElement = doc.createElement("GetFederationTokenResponse");
        Attr attr = doc.createAttribute("xmlns");
        attr.setValue(IAM_XMLNS);
        responseElement.setAttributeNode(attr);
        doc.appendChild(responseElement);

        Element resultElement = doc.createElement("GetFederationTokenResult");
        responseElement.appendChild(resultElement);

        Element credentialElement = doc.createElement("Credentials");
        resultElement.appendChild(credentialElement);

        for (Map.Entry<String, String> entry : credentials.entrySet()) {
            Element element = doc.createElement(entry.getKey());
            element.appendChild(doc.createTextNode(entry.getValue()));
            credentialElement.appendChild(element);
        }

        Element federatedUserElement = doc.createElement("FederatedUser");
        resultElement.appendChild(federatedUserElement);

        for (Map.Entry<String, String> entry : federatedUser.entrySet()) {
            Element element = doc.createElement(entry.getKey());
            element.appendChild(doc.createTextNode(entry.getValue()));
            federatedUserElement.appendChild(element);
        }

        Element packedPolicySizeElement = doc.createElement("PackedPolicySize");
        packedPolicySizeElement.appendChild(doc.createTextNode(packedPolicy));
        resultElement.appendChild(packedPolicySizeElement);

        Element responseMetaData = doc.createElement("ResponseMetadata");
        responseElement.appendChild(responseMetaData);

        Element requestIdElement = doc.createElement("RequestId");
        requestIdElement.appendChild(doc.createTextNode(requestId));
        responseMetaData.appendChild(requestIdElement);

        String responseBody;
        try {
            responseBody = docToString(doc);
            ServerResponse serverResponse = new ServerResponse(HttpResponseStatus.CREATED,
                    responseBody);

            return serverResponse;
        } catch (TransformerException ex) {
        }

        return null;
    }
}
