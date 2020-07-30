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

public class AssumeRoleWithSAMLResponseFormatter extends XMLResponseFormatter {

    @Override
    public ServerResponse formatCreateResponse(String operation, String returnObject,
            LinkedHashMap<String, String> responseElements, String requestId) {
        throw new UnsupportedOperationException("Not supported yet.");
    }

    public ServerResponse formatCreateResponse(LinkedHashMap<String, String> credentials,
            LinkedHashMap<String, String> federatedUser,
            LinkedHashMap<String, String> samlAttributes,
            String packedPolicy, String requestId) {
        Document doc;
        try {
            doc = createNewDoc();
        } catch (ParserConfigurationException ex) {
            return null;
        }

        Element rootElement = doc.createElement("AssumeRoleWithSAMLResponse");
        Attr attr = doc.createAttribute("xmlns");
        attr.setValue(IAM_XMLNS);
        rootElement.setAttributeNode(attr);
        doc.appendChild(rootElement);

        Element result = doc.createElement("AssumeRoleWithSAMLResult");
        rootElement.appendChild(result);

        Element credentialElement = doc.createElement("Credentials");
        result.appendChild(credentialElement);

        for (Map.Entry<String, String> entry : credentials.entrySet()) {
            Element element = doc.createElement(entry.getKey());
            element.appendChild(doc.createTextNode(entry.getValue()));
            credentialElement.appendChild(element);
        }

        Element assumedRoleUserElement = doc.createElement("AssumedRoleUser");
        result.appendChild(assumedRoleUserElement);

        for (Map.Entry<String, String> entry : federatedUser.entrySet()) {
            Element element = doc.createElement(entry.getKey());
            element.appendChild(doc.createTextNode(entry.getValue()));
            assumedRoleUserElement.appendChild(element);
        }

        for (Map.Entry<String, String> entry : samlAttributes.entrySet()) {
            Element element = doc.createElement(entry.getKey());
            element.appendChild(doc.createTextNode(entry.getValue()));
            result.appendChild(element);
        }

        Element packedPolicySize = doc.createElement("PackedPolicySize");
        packedPolicySize.appendChild(doc.createTextNode(packedPolicy));
        result.appendChild(packedPolicySize);

        Element responseMetaData = doc.createElement("ResponseMetadata");
        rootElement.appendChild(responseMetaData);

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
