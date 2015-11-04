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
 * Original creation date: 29-October-2015
 */

package com.seagates3.model;

import java.security.cert.Certificate;
import org.joda.time.DateTime;

public class SAMLResponse {

    private String audience;
    private Certificate certificate;
    private String destination;
    private String issuer;
    private DateTime issueInstant;
    private DateTime notBefore;
    private DateTime notOnOrAfter;
    private String roleSessionName;
    private String subject;
    private Boolean success;
    private String subjectType;
    private Boolean validSignature;

    public String getAudience() {
        return audience;
    }

    public Certificate getCertificate() {
        return certificate;
    }

    public String getDestination() {
        return destination;
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

    public String getRoleSessionName() {
        return roleSessionName;
    }

    public Boolean isSuccess() {
        return success;
    }

    public String getsubject() {
        return subject;
    }

    public String getsubjectType() {
        return subjectType;
    }

    public Boolean isValidSignature() {
        return validSignature;
    }

    public void setAudience(String audience) {
        this.audience = audience;
    }

    public void setCertificate(Certificate certificate) {
        this.certificate = certificate;
    }

    public void setDestination(String destination) {
        this.destination = destination;
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

    public void setRoleSessionName(String roleSessionName) {
        this.roleSessionName = roleSessionName;
    }

    public void setSuccess(Boolean success) {
        this.success = success;
    }

    public void setSubject(String subject) {
        this.subject = subject;
    }

    public void setSubjectType(String subjectType) {
        this.subjectType = subjectType;
    }

    public void setValidSignature(Boolean validSignature) {
        this.validSignature = validSignature;
    }
}
