package com.seagates3.saml;

import com.seagates3.exception.SAMLInitializationException;

public class SAMLUtilFactory {

    private final static String SAML2_NAMESPACE
            = "urn:oasis:names:tc:SAML:2.0";

    public static SAMLUtil getSAMLUtil(String SAMLResponse)
            throws SAMLInitializationException {
        if (SAMLResponse.contains(SAML2_NAMESPACE)) {
            return new SAMLUtilV2();
        }
        return null;
    }
}
