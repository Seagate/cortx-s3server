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

package com.seagates3.util;

import static org.junit.Assert.*;

import org.junit.Test;
import org.xml.sax.SAXException;

public class XMLValidatorUtilTest {

    String xsdPath = "../resources/AmazonS3.xsd";

    String acl = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" +
            "<AccessControlPolicy"
            + " xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">"
            + "<Owner><ID>"
            + "b103e16d027d24270d8facf37a48b141fd88ac8f43f9f942b91ba1cf1dc33f71"
            + "</ID>"
            + "<DisplayName>kirungeb</DisplayName></Owner><AccessControlList>"
            + "<Grant>"
            + "<Grantee "
            + "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" "
            + "xsi:type=\"CanonicalUser\"><ID>"
            + "b103e16d027d24270d8facf37a48b141fd88ac8f43f9f942b91ba1cf1dc33f71"
            + "</ID>"
            + "<DisplayName>kirungeb</DisplayName></Grantee>"
            + "<Permission>FULL_CONTROL</Permission></Grant></AccessControlList>"
            + "</AccessControlPolicy>";

    String acl_invalid_permission =
            "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
            + "<AccessControlPolicy"
            + " xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">"
            + "<Owner><ID>"
            + "b103e16d027d24270d8facf37a48b141fd88ac8f43f9f942b91ba1cf1dc33f71"
            + "</ID>"
            + "<DisplayName>kirungeb</DisplayName></Owner><AccessControlList>"
            + "<Grant>"
            + "<Grantee "
            + "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" "
            + "xsi:type=\"CanonicalUser\"><ID>"
            + "b103e16d027d24270d8facf37a48b141fd88ac8f43f9f942b91ba1cf1dc33f71"
            + "</ID>"
            + "<DisplayName>kirungeb</DisplayName></Grantee>"
            + "<Permission>CONTROL</Permission></Grant></AccessControlList>"
            + "</AccessControlPolicy>";

    String acl_invalid_missingid =
            "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
            + "<AccessControlPolicy"
            + " xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">"
            + "<Owner><ID>"
            + "b103e16d027d24270d8facf37a48b141fd88ac8f43f9f942b91ba1cf1dc33f71"
            + "</ID>"
            + "<DisplayName>kirungeb</DisplayName></Owner><AccessControlList>"
            + "<Grant>"
            + "<Grantee "
            + "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" "
            + "xsi:type=\"CanonicalUser\">"
            + "<DisplayName>kirungeb</DisplayName></Grantee>"
            + "<Permission>READ</Permission></Grant></AccessControlList>"
            + "</AccessControlPolicy>";

    String acl_invalid_nograntee =
            "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
            + "<AccessControlPolicy"
            + " xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">"
            + "<Owner><ID>"
            + "b103e16d027d24270d8facf37a48b141fd88ac8f43f9f942b91ba1cf1dc33f71"
            + "</ID>"
            + "<DisplayName>kirungeb</DisplayName></Owner><AccessControlList>"
            + "<Grant>"
            + "<Permission>READ</Permission></Grant></AccessControlList>"
            + "</AccessControlPolicy>";


    String acl_missing_owner = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" +
            "<AccessControlPolicy"
            + " xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">"
            + "<AccessControlList>"
            + "<Grant>"
            + "<Grantee "
            + "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" "
            + "xsi:type=\"CanonicalUser\"><ID>"
            + "b103e16d027d24270d8facf37a48b141fd88ac8f43f9f942b91ba1cf1dc33f71"
            + "</ID>"
            + "<DisplayName>kirungeb</DisplayName></Grantee>"
            + "<Permission>FULL_CONTROL</Permission></Grant></AccessControlList>"
            + "</AccessControlPolicy>";

    String acl_no_permission = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" +
            "<AccessControlPolicy"
            + " xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">"
            + "<Owner><ID>"
            + "b103e16d027d24270d8facf37a48b141fd88ac8f43f9f942b91ba1cf1dc33f71"
            + "</ID>"
            + "<DisplayName>kirungeb</DisplayName></Owner><AccessControlList>"
            + "<Grant>"
            + "<Grantee "
            + "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" "
            + "xsi:type=\"CanonicalUser\"><ID>"
            + "b103e16d027d24270d8facf37a48b141fd88ac8f43f9f942b91ba1cf1dc33f71"
            + "</ID>"
            + "<DisplayName>kirungeb</DisplayName></Grantee>"
            + "</Grant></AccessControlList>"
            + "</AccessControlPolicy>";

    String acl_multiple_grant = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" +
            "<AccessControlPolicy"
            + " xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">"
            + "<Owner><ID>"
            + "b103e16d027d24270d8facf37a48b141fd88ac8f43f9f942b91ba1cf1dc33f71"
            + "</ID>"
            + "<DisplayName>kirungeb</DisplayName></Owner><AccessControlList>"
            + "<Grant>"
            + "<Grantee "
            + "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" "
            + "xsi:type=\"CanonicalUser\"><ID>"
            + "b103e16d027d24270d8facf37a48b141fd88ac8f43f9f942b91ba1cf1dc33f71"
            + "</ID>"
            + "<DisplayName>kirungeb</DisplayName></Grantee>"
            + "<Permission>READ</Permission></Grant>"
            + "<Grant><Grantee "
            + "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" "
            + "xsi:type=\"CanonicalUser\"><ID>"
            + "24270d8facf37a48b141fd88ac8f43f9f942b91ba1cf1dc33f71b103e16d027d"
            + "</ID>"
            + "<DisplayName>myapp</DisplayName></Grantee>"
            + "<Permission>READ</Permission></Grant></AccessControlList>"
            + "</AccessControlPolicy>";

    String acl_malformed = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" +
            "<AccessControlPolicy"
            + " xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">"
            + "<Owner><ID>"
            + "b103e16d027d24270d8facf37a48b141fd88ac8f43f9f942b91ba1cf1dc33f71"
            + "</ID>"
            + "<DisplayName>kirungeb</DisplayName></Owner><AccessControlList>"
            + "<Grant>"
            + "<Grantee "
            + "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" "
            + "xsi:type=\"CanonicalUser\"><ID>"
            + "b103e16d027d24270d8facf37a48b141fd88ac8f43f9f942b91ba1cf1dc33f71"
            + "</ID>"
            + "<DisplayName>kirungeb</DisplayName></Grantee>"
            + "<Permission>READ</Permission></Grant>"
            + "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" "
            + "xsi:type=\"CanonicalUser\"><ID>"
            + "24270d8facf37a48b141fd88ac8f43f9f942b91ba1cf1dc33f71b103e16d027d"
            + "</ID>"
            + "<DisplayName>myapp</DisplayName></Grantee>"
            + "<Permission>READ</Permission></Grant></AccessControlList>"
            + "</AccessControlPolicy>";

    String acl_empty = "";

    String acl_null = null;

    String acl_namespace_ver = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" +
            "<AccessControlPolicy"
            + " xmlns=\"http://s3.amazonaws.com/doc/2010-03-01/\">"
            + "<Owner><ID>"
            + "b103e16d027d24270d8facf37a48b141fd88ac8f43f9f942b91ba1cf1dc33f71"
            + "</ID>"
            + "<DisplayName>kirungeb</DisplayName></Owner><AccessControlList>"
            + "<Grant>"
            + "<Grantee "
            + "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" "
            + "xsi:type=\"CanonicalUser\"><ID>"
            + "b103e16d027d24270d8facf37a48b141fd88ac8f43f9f942b91ba1cf1dc33f71"
            + "</ID>"
            + "<DisplayName>kirungeb</DisplayName></Grantee>"
            + "<Permission>FULL_CONTROL</Permission></Grant></AccessControlList>"
            + "</AccessControlPolicy>";

    @Test
    public void testValidACL() {
        try {
            XMLValidatorUtil xmlValidator = new XMLValidatorUtil(xsdPath);
            assertTrue(xmlValidator.validateXMLSchema(acl));
        } catch(SAXException ex) {
            fail("Failed to initialize XSD file "+ " xsdPath" + ex.getMessage());
        }
    }

    @Test
    public void testInvalidPermissionACL() {
           try {
               XMLValidatorUtil xmlValidator = new XMLValidatorUtil(xsdPath);
               assertFalse(xmlValidator.validateXMLSchema(acl_invalid_permission));
           } catch(SAXException ex) {
               fail("Failed to initialize XSD file "+ " xsdPath" + ex.getMessage());
        }
    }

    @Test
    public void testInvalidMissingID() {
           try {
               XMLValidatorUtil xmlValidator = new XMLValidatorUtil(xsdPath);
               assertFalse(xmlValidator.validateXMLSchema(acl_invalid_missingid));
           } catch(SAXException ex) {
               fail("Failed to initialize XSD file "+ " xsdPath" + ex.getMessage());
        }
    }

    @Test
    public void testInvalidNoGrantee() {
           try {
               XMLValidatorUtil xmlValidator = new XMLValidatorUtil(xsdPath);
               assertFalse(xmlValidator.validateXMLSchema(acl_invalid_nograntee));
           } catch(SAXException ex) {
               fail("Failed to initialize XSD file "+ " xsdPath" + ex.getMessage());
        }
    }

    @Test
    public void testInvalidNoOwner() {
        try {
              XMLValidatorUtil xmlValidator = new XMLValidatorUtil(xsdPath);
               assertFalse(xmlValidator.validateXMLSchema(acl_missing_owner));
           } catch(SAXException ex) {
               fail("Failed to initialize XSD file "+ " xsdPath" + ex.getMessage());
        }
    }

    @Test
    public void testInvalidNoPermission() {
           try {
               XMLValidatorUtil xmlValidator = new XMLValidatorUtil(xsdPath);
               assertFalse(xmlValidator.validateXMLSchema(acl_no_permission));
           } catch(SAXException ex) {
               fail("Failed to initialize XSD file "+ " xsdPath" + ex.getMessage());
        }
    }

    @Test
    public void testValidACLMultiple() {
           try {
               XMLValidatorUtil xmlValidator = new XMLValidatorUtil(xsdPath);
               assertTrue(xmlValidator.validateXMLSchema(acl_multiple_grant));
           } catch(SAXException ex) {
               fail("Failed to initialize XSD file "+ " xsdPath" + ex.getMessage());
        }
    }

    @Test
    public void testACLMalformed() {
           try {
               XMLValidatorUtil xmlValidator = new XMLValidatorUtil(xsdPath);
               assertFalse(xmlValidator.validateXMLSchema(acl_malformed));
           } catch(SAXException ex) {
               fail("Failed to initialize XSD file "+ " xsdPath" + ex.getMessage());
        }
    }

    @Test
    public void testEmptyACL() {
           try {
               XMLValidatorUtil xmlValidator = new XMLValidatorUtil(xsdPath);
               assertFalse(xmlValidator.validateXMLSchema(acl_empty));
           } catch(SAXException ex) {
               fail("Failed to initialize XSD file "+ " xsdPath" + ex.getMessage());
        }
    }

    @Test
    public void testInvalidVerACL() {
           try {
               XMLValidatorUtil xmlValidator = new XMLValidatorUtil(xsdPath);
               assertFalse(xmlValidator.validateXMLSchema(acl_namespace_ver));
           } catch(SAXException ex) {
               fail("Failed to initialize XSD file "+ " xsdPath" + ex.getMessage());
        }
    }

    @Test
    public void testMissingXSD() {
           try {
               XMLValidatorUtil xmlValidator = new XMLValidatorUtil("filenotfound.notfound");
               assertFalse(xmlValidator.validateXMLSchema(acl));
           } catch(SAXException ex) {
               assertTrue(ex.getMessage(), true);
        }
    }

    @Test
    public void testNullACL() {
           try {
               XMLValidatorUtil xmlValidator = new XMLValidatorUtil(xsdPath);
               assertFalse(xmlValidator.validateXMLSchema(acl_null));
           } catch(SAXException ex) {
               assertTrue(ex.getMessage(), true);
        }
    }

}
