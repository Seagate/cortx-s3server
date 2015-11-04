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
 * Original creation date: 15-Oct-2015
 */

package com.seagates3.util;

import java.io.ByteArrayInputStream;
import java.io.InputStream;
import java.nio.charset.StandardCharsets;
import java.security.cert.Certificate;
import java.security.cert.CertificateException;
import java.security.cert.CertificateFactory;
import java.util.ArrayList;
import java.util.Calendar;
import java.util.List;
import java.util.TimeZone;

import org.opensaml.DefaultBootstrap;
import org.opensaml.common.impl.SecureRandomIdentifierGenerator;
import org.opensaml.saml2.core.Assertion;
import org.opensaml.saml2.core.Attribute;
import org.opensaml.saml2.core.AttributeStatement;
import org.opensaml.saml2.core.Response;
import org.opensaml.saml2.core.Subject;
import org.opensaml.saml2.core.SubjectConfirmation;
import org.opensaml.saml2.metadata.EntityDescriptor;
import org.opensaml.saml2.metadata.IDPSSODescriptor;
import org.opensaml.saml2.metadata.NameIDFormat;
import org.opensaml.saml2.metadata.SingleLogoutService;
import org.opensaml.saml2.metadata.SingleSignOnService;
import org.opensaml.security.SAMLSignatureProfileValidator;
import org.opensaml.xml.Configuration;
import org.opensaml.xml.io.Unmarshaller;
import org.opensaml.xml.io.UnmarshallerFactory;
import org.opensaml.xml.io.UnmarshallingException;
import org.opensaml.xml.parse.BasicParserPool;
import org.opensaml.xml.parse.XMLParserException;
import org.opensaml.xml.schema.XSAny;
import org.opensaml.xml.security.credential.BasicCredential;
import org.opensaml.xml.signature.KeyInfo;
import org.opensaml.xml.signature.Signature;
import org.opensaml.xml.signature.SignatureValidator;
import org.opensaml.xml.signature.X509Certificate;
import org.opensaml.xml.signature.X509Data;
import org.opensaml.xml.validation.ValidationException;
import org.opensaml.xml.XMLObject;

import org.w3c.dom.Document;
import org.w3c.dom.Element;

import org.joda.time.DateTime;

import com.seagates3.model.SAMLProvider;
import com.seagates3.model.KeyDescriptor;
import com.seagates3.model.SAMLAssertionAttribute;
import com.seagates3.model.SAMLResponse;
import com.seagates3.model.SAMLServiceEndpoints;

public class SAMLUtil {

    private static SecureRandomIdentifierGenerator generator;
    private static BasicParserPool parser;
    private static UnmarshallerFactory unmarshallerFactory;

    private static final String SUCCESS = "urn:oasis:names:tc:SAML:2.0:status:Success";
    private static final String IDP_SSO_DESCRIPTOR = "urn:oasis:names:tc:SAML:2.0:protocol";
    private static final String SUBJECT_CONFIRMATION_BEARER = "urn:oasis:names:tc:SAML:2.0:cm:bearer";
    private static final String Attribute_ROLE_SESSION_NAME = "https://aws.amazon.com/SAML/Attributes/RoleSessionName";
    private static final String PERSISTENT_SUBJECT_TYPE = "urn:oasis:names:tc:SAML:2.0:nameid-format:persistent";

    public static String samlMetadataFile;

    public static void init() {
        try {
            DefaultBootstrap.bootstrap();
            generator = new SecureRandomIdentifierGenerator ();
            parser = new BasicParserPool();
            parser.setNamespaceAware(true);
            unmarshallerFactory = Configuration.getUnmarshallerFactory();
        } catch (Exception ex) {
            System.out.println(ex);
        }
    }

    public static void setSamlMetadataFile(String filePath) {
        samlMetadataFile = filePath;
    }

    public static SAMLProvider getsamlProvider(SAMLProvider samlProvider, String metadata)
            throws XMLParserException, UnmarshallingException, CertificateException {
        EntityDescriptor entityDescriptor = getEntityDescriptor(metadata);
        samlProvider.setIssuer(entityDescriptor.getEntityID());

        IDPSSODescriptor iDescriptor =
                entityDescriptor.getIDPSSODescriptor(IDP_SSO_DESCRIPTOR);

        samlProvider.setExpiry(getProviderExpiration(entityDescriptor));
        samlProvider.setKeyDescriptors(getKeyDescriptors(iDescriptor));

        /*
         * TODO
         * Set saml service, name id formats and attributes.
         * This will be required for single singon and single log out service.
         */
        // samlProvider.setSAMLServices(getSAMLServices(iDescriptor));
        // samlProvider.setNameIdFormats(getNameIdFormats(iDescriptor));
        // samlProvider.setAttributes(getSAMLAssertionAttributes(iDescriptor));

        return samlProvider;
    }

    /*
     * Parse the SAML body and return SAML response object.
     */
    public static SAMLResponse getSamlResponse(String responseMessage) {
        Response response;
        try {
            response = unmarshallResponse(responseMessage);
        } catch (XMLParserException | UnmarshallingException ex) {
            return null;
        }
        SAMLResponse samlResponse = new SAMLResponse();

        Boolean reponseSuccess = response.getStatus().getStatusCode().getValue().equals(SUCCESS);
        samlResponse.setSuccess(reponseSuccess);

        /*
         * TODO
         * Iterate over all the assertions.
         */
        Assertion assertion = response.getAssertions().get(0);
        Signature sign= assertion.getSignature();

        assertion.getSignature().getKeyInfo();
        samlResponse.setIssuer(response.getIssuer().getValue());
        samlResponse.setDestination(response.getDestination());

        Certificate certificate;
        try {
            certificate = getCertificate(sign.getKeyInfo());
            samlResponse.setCertificate(certificate);
        } catch (CertificateException ex) {
            return null;
        }

        samlResponse.setValidSignature(isResponseValid(sign, certificate));
        samlResponse.setIssueInstant(assertion.getIssueInstant());

        Subject subject = assertion.getSubject();
        samlResponse.setSubject(subject.getNameID().getValue());

        String subjectType = getSubjectType(subject.getNameID().getFormat());
        samlResponse.setSubjectType(subjectType);

        List<SubjectConfirmation> subjectConfirmations = subject.getSubjectConfirmations();
        /*
         * TODO
         * Support Holder of Key and Sender Vouches subject confirmation methods.
         */
        for(SubjectConfirmation s : subjectConfirmations) {
            if(s.getMethod().equals(SUBJECT_CONFIRMATION_BEARER)) {
                samlResponse.setNotBefore(s.getSubjectConfirmationData().getNotBefore());
                samlResponse.setNotOnOrAfter(s.getSubjectConfirmationData().getNotOnOrAfter());
                break;
            }
        }

        samlResponse.setRoleSessionName(getRoleSessionName(assertion.getAttributeStatements()));
        samlResponse.setAudience(response.getDestination());

        return samlResponse;
    }

    /*
     * Return the Role session name
     */
    private static String getRoleSessionName(List<AttributeStatement> attributeStatements) {
        for(AttributeStatement as : attributeStatements) {
            List<Attribute> attributes = as.getAttributes();
            for(Attribute a : attributes) {
                if(a.getName().equals(Attribute_ROLE_SESSION_NAME)) {
                    List<XMLObject> attributeValues = a.getAttributeValues();
                    XSAny xs = (XSAny) attributeValues.get(0);
                    return xs.getTextContent();
                }
            }
        }

        return null;
    }

    /*
     * Return the subject type
     */
    private static String getSubjectType(String subjectType) {
        if(PERSISTENT_SUBJECT_TYPE.equals(subjectType)) {
            return "persistent";
        } else {
            return "transcient";
        }
    }

    /*
     * Return the entity descriptor object.
     */
    private static EntityDescriptor getEntityDescriptor(String metadata)
            throws XMLParserException, UnmarshallingException {
        InputStream inStream = new ByteArrayInputStream(metadata.getBytes(StandardCharsets.UTF_8));
        Document inCommonMDDoc = parser.parse(inStream);
        Element metadataRoot = inCommonMDDoc.getDocumentElement();

        Unmarshaller unmarshaller = unmarshallerFactory.getUnmarshaller(metadataRoot);

        return (EntityDescriptor) unmarshaller.unmarshall(metadataRoot);
    }

    /*
     * Return the expiration date of the provider.
     * If expiration is not provided in the metadata,
     *   Then set expiration after 100 years. (Default AWS behaviour).
     */
    private static String getProviderExpiration(EntityDescriptor entityDescriptor) {
        DateTime providerExpiry = entityDescriptor.getValidUntil();
        if(providerExpiry == null) {
            Calendar cal = Calendar.getInstance(TimeZone.getTimeZone("UTC"));
            cal.add(Calendar.YEAR, 100);
            return DateUtil.toLdapDate(cal.getTime());
        } else {
            return DateUtil.toLdapDate(providerExpiry.toDate());
        }
    }

    /*
     * Iterate over the metadata and generate key descriptor objects.
     */
    private static KeyDescriptor[] getKeyDescriptors(IDPSSODescriptor iDescriptor)
            throws CertificateException {
        ArrayList<KeyDescriptor> keyDescriptors = new ArrayList<>();

        for(org.opensaml.saml2.metadata.KeyDescriptor k : iDescriptor.getKeyDescriptors()) {
            KeyDescriptor keyDescriptor = new KeyDescriptor();
            keyDescriptor.setUse(KeyDescriptor.KeyUse.valueOf(k.getUse().toString().toUpperCase()));

            ArrayList<Certificate> certificates = new ArrayList<>();
            for(X509Data x509Data : k.getKeyInfo().getX509Datas()) {
                for(X509Certificate cert : x509Data.getX509Certificates()) {
                    certificates.add(toCertificate(cert));
                }
            }

            keyDescriptor.setCertificate(certificates.toArray(new Certificate[certificates.size()]));
            keyDescriptors.add(keyDescriptor);
        }

        return keyDescriptors.toArray(new KeyDescriptor[keyDescriptors.size()]);
    }

    /*
     * Return the certificate from Key Info
     */
    private static Certificate getCertificate(KeyInfo keyInfo) throws CertificateException {
        Certificate certificate = null;
        for(X509Data x509Data : keyInfo.getX509Datas()) {
            for(X509Certificate cert : x509Data.getX509Certificates()) {
                certificate = toCertificate(cert);
            }
        }

        return certificate;
    }

    /*
     * Convert Opensaml x509 Certificate to java.security.cert.certificate
     */
    private static Certificate toCertificate(X509Certificate cert)
            throws CertificateException {
        byte[] decode = BinaryUtil.base64DecodedBytes(cert.getValue());
        return CertificateFactory.getInstance("X.509").generateCertificate(new ByteArrayInputStream(decode));
    }

    /*
     * Return is the SAML signature is valid.
     */
    private static Boolean isResponseValid(Signature signature, Certificate cert) {
        SAMLSignatureProfileValidator profileValidator = new SAMLSignatureProfileValidator();
        try {
            profileValidator.validate(signature);
        } catch (ValidationException ex) {
            return false;
        }

        BasicCredential cred1 = new BasicCredential();
        cred1.setPublicKey(cert.getPublicKey());

        SignatureValidator sigValidator = new SignatureValidator(cred1);
        try {
            sigValidator.validate(signature);
        } catch (ValidationException ex) {
            return false;
        }

        return true;
    }

    /*
     * Get the end points of single sin on and single log out services.
     */
    private static SAMLServiceEndpoints[] getSAMLServices(IDPSSODescriptor iDescriptor) {
        ArrayList<SAMLServiceEndpoints> samlServiceEndpoints = new ArrayList<>();

        for(SingleLogoutService sLogOut : iDescriptor.getSingleLogoutServices()) {
            SAMLServiceEndpoints serviceEndpoint = new SAMLServiceEndpoints();

            if(sLogOut.getBinding().contains("POST")) {
                serviceEndpoint.setBinding(SAMLServiceEndpoints.Binding.HTTP_POST);
            } else {
                serviceEndpoint.setBinding(SAMLServiceEndpoints.Binding.HTTP_REDIRECT);
            }

            serviceEndpoint.setServiceType(SAMLServiceEndpoints.ServiceType.SINGLE_LOGOUT_SERVICE);
            serviceEndpoint.setLocation(sLogOut.getLocation());

            samlServiceEndpoints.add(serviceEndpoint);
        }

        for(SingleSignOnService sSignOn : iDescriptor.getSingleSignOnServices()) {
            SAMLServiceEndpoints service = new SAMLServiceEndpoints();

            if(sSignOn.getBinding().contains("POST")) {
                service.setBinding(SAMLServiceEndpoints.Binding.HTTP_POST);
            } else {
                service.setBinding(SAMLServiceEndpoints.Binding.HTTP_REDIRECT);
            }

            service.setServiceType(SAMLServiceEndpoints.ServiceType.SINGLE_SIGNON_SERVICE);
            service.setLocation(sSignOn.getLocation());

            samlServiceEndpoints.add(service);
        }

        return (SAMLServiceEndpoints[]) samlServiceEndpoints.toArray();
    }

    /*
     * Get name id format from metadata.
     */
    private static String[] getNameIdFormats(IDPSSODescriptor iDescriptor) {
        ArrayList<String> nameIdFormats = new ArrayList<>();

        for(NameIDFormat n : iDescriptor.getNameIDFormats()) {
            nameIdFormats.add(n.getFormat());
        }

        return (String[]) nameIdFormats.toArray();
    }

    /*
     * Get SAML Assertion attributes from metadata.
     */
    private static SAMLAssertionAttribute[] getSAMLAssertionAttributes(IDPSSODescriptor iDescriptor) {
        ArrayList<SAMLAssertionAttribute> attributes = new ArrayList<>();

        for(Attribute a : iDescriptor.getAttributes()) {
            SAMLAssertionAttribute attribute = new SAMLAssertionAttribute();
            attribute.setFriendlyName(a.getFriendlyName());
            attribute.setName(a.getName());

            attributes.add(attribute);
        }

        return (SAMLAssertionAttribute[]) attributes.toArray();
    }

    /*
     * Unmarshall the response to opensaml response object.
     */
    private static Response unmarshallResponse(String response)
            throws XMLParserException, UnmarshallingException {
        InputStream in = new ByteArrayInputStream(response.getBytes(StandardCharsets.UTF_8));
        Document responseDoc = parser.parse(in);
        Element responseRoot = responseDoc.getDocumentElement();

        Unmarshaller unmarshaller = unmarshallerFactory.getUnmarshaller(responseRoot);
        return (Response) unmarshaller.unmarshall(responseRoot);
    }
}
