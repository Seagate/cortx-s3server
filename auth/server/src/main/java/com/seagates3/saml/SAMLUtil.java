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

package com.seagates3.saml;

import com.seagates3.exception.SAMLInvalidCertificateException;
import com.seagates3.exception.SAMLReponseParserException;
import com.seagates3.model.SAMLProvider;
import com.seagates3.model.SAMLResponseTokens;
import java.util.ArrayList;
import java.util.Map;
import javax.management.InvalidAttributeValueException;
import org.opensaml.xml.signature.Signature;

public interface SAMLUtil {

    /**
     * Maximum time for credential expiry. Time is in seconds.
     */
    public int maxCredsDuration = 3600;

    public final String ATTRIBUTE_ROLE_NAME
            = "https://s3.seagate.com/SAML/Attributes/Role";

    public final String ATTRIBUTE_ROLE_SESSION_NAME
            = "https://s3.seagate.com/SAML/Attributes/RoleSessionName";

    public final String AUDIENCE_NAME = "urn:seagate:webservices";

    /**
     * Parse the metadata of SAML Provider and create SAML provider object.
     *
     * @param samlProvider SAMLProvider object.
     * @param samlMetadata IDP SAML metadata.
     * @throws SAMLReponseParserException
     * @throws InvalidAttributeValueException
     * @throws SAMLInvalidCertificateException
     */
    public void getSAMLProvider(SAMLProvider samlProvider, String samlMetadata)
            throws SAMLReponseParserException, InvalidAttributeValueException,
            SAMLInvalidCertificateException;

    /**
     * Parse SAML Response message into Response tokens.
     *
     * @param samlResponseMessage SAML response message.
     * @return SAMLResponseTokens
     * @throws SAMLReponseParserException
     * @throws SAMLInvalidCertificateException
     */
    public SAMLResponseTokens parseSamlResponse(String samlResponseMessage)
            throws SAMLReponseParserException, SAMLInvalidCertificateException;

    /**
     * Validate signature of the SAML response.
     *
     * @param signature SAML signature
     * @param SigningCert Certificate used to sign the signature.
     * @return True or False
     * @throws SAMLInvalidCertificateException
     */
    public Boolean isResponseProfileValid(Signature signature,
            String SigningCert) throws SAMLInvalidCertificateException;

    /**
     * SAML response may or may not contain the signing certificate. If signing
     * certificate is not present in the SAML response, then fetch the signing
     * certificates from the IDP metadata and check if the SAML response is
     * valid.
     *
     * @param signature
     * @param samlProvider
     * @return
     * @throws com.seagates3.exception.SAMLInvalidCertificateException
     */
    public Boolean isResponseProfileValid(Signature signature,
            SAMLProvider samlProvider) throws SAMLInvalidCertificateException;

    /**
     * Get the role session name.
     *
     * @param responseAttributes SAML response attributes.
     * @return Role session name.
     */
    public String getRoleSessionName(
            Map<String, ArrayList<String>> responseAttributes);

    /**
     * Return the roles from SAML Response attributes.
     *
     * @param responseAttributes
     * @return
     */
    public ArrayList<String> getRoles(
            Map<String, ArrayList<String>> responseAttributes);

    /**
     * Calculate the duration for which the credentials are valid.
     *
     * @param samlResponseTokens SAML response tokens.
     * @param requestDuration User Requested Duration.
     * @return Expiration duration in seconds.
     */
    public int getExpirationDuration(SAMLResponseTokens samlResponseTokens,
            int requestDuration);
}
