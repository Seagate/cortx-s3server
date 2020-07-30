package com.seagates3.model;

import java.util.ArrayList;
import java.util.Map;
import org.joda.time.DateTime;
import org.opensaml.xml.signature.Signature;

public class SAMLResponseTokens {

    private Boolean isAuthenticationSuccess;
    private Boolean isResponseAudience;
    private DateTime issueInstant;
    private DateTime notBefore;
    private DateTime notOnOrAfter;
    private Map<String, ArrayList<String>> responseAttributes;
    private Signature responseSignature;
    private String audience;
    private String issuer;
    private String signingCertificate;
    private String subject;
    private String subjectType;

    public String getAudience() {
        return audience;
    }

    public String getSigningCertificate() {
        return signingCertificate;
    }

    public String getIssuer() {
        return issuer;
    }

    public DateTime getIssueInstant() {
        return issueInstant;
    }

    public DateTime getNotBefore() {
        return notBefore;
    }

    public DateTime getNotOnOrAfter() {
        return notOnOrAfter;
    }

    public Boolean isAuthenticationSuccess() {
        return isAuthenticationSuccess;
    }

    public Boolean isResponseAudience() {
        return isResponseAudience;
    }

    public String getsubject() {
        return subject;
    }

    public String getsubjectType() {
        return subjectType;
    }

    public Map<String, ArrayList<String>> getResponseAttributes() {
        return responseAttributes;
    }

    public Signature getResponseSignature() {
        return responseSignature;
    }

    public void setAudience(String audience) {
        this.audience = audience;
    }

    public void setSigningCertificate(String certificate) {
        this.signingCertificate = certificate;
    }

    public void setIssuer(String issuer) {
        this.issuer = issuer;
    }

    public void setIssueInstant(DateTime issueInstant) {
        this.issueInstant = issueInstant;
    }

    public void setNotBefore(DateTime notBefore) {
        this.notBefore = notBefore;
    }

    public void setNotOnOrAfter(DateTime notOnOrAfter) {
        this.notOnOrAfter = notOnOrAfter;
    }

    public void setIsAuthenticationSuccess(Boolean success) {
        this.isAuthenticationSuccess = success;
    }

    public void setIsResponseAudience(Boolean isResponseAudience) {
        this.isResponseAudience = isResponseAudience;
    }

    public void setSubject(String subject) {
        this.subject = subject;
    }

    public void setSubjectType(String subjectType) {
        this.subjectType = subjectType;
    }

    public void setResponseSignature(Signature responseSignature) {
        this.responseSignature = responseSignature;
    }

    public void setResponseAttributes(
            Map<String, ArrayList<String>> responseAttributes) {
        this.responseAttributes = responseAttributes;
    }
}
