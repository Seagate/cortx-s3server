/*
 * COPYRIGHT 2019 SEAGATE LLC
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
 * Original author:  Abhilekh Mustapure <abhilekh.mustapure@seagate.com>
 * Original creation date: 05-Apr-2019
 */


package com.seagates3.authorization;

import com.seagates3.authorization.AccessControlPolicy;
import org.junit.rules.ExpectedException;
import org.junit.Rule;
import org.junit.Test;
import org.xml.sax.SAXException;
import javax.xml.parsers.ParserConfigurationException;
import java.io.IOException;
import org.junit.Before;
import com.seagates3.authorization.AccessControlList;


import static org.junit.Assert.*;

public class AccessControlPolicyTest {

    /*
     *  Set up for tests
     */

    String aclXmlPath = "../resources/defaultAclTemplate.xml";
    AccessControlPolicy accessControlPolicy;

    @Before
    public void setUp() throws ParserConfigurationException, SAXException, IOException {
        accessControlPolicy = new AccessControlPolicy(aclXmlPath);
    }

    @Test
    public void loadxml_OwnerId_Test() {
        Owner owner = accessControlPolicy.getOwner();
        assertEquals(owner.canonicalId,"Owner_ID");
    }

    @Test
    public void loadxml_OwnerName_Test() {
        Owner owner = accessControlPolicy.getOwner();
        assertEquals(owner.displayName,"Owner_Name");

    }

    @Test
    public void loadxml_GranteeName_Test() {
        AccessControlList accessControlList = accessControlPolicy.getAccessControlList();
        assertEquals(accessControlList.grantList.get(0).grantee.displayName, "Grantee_Name");

    }

    @Test
    public void loadxml_GranteeId_Test() {
        AccessControlList accessControlList = accessControlPolicy.getAccessControlList();
        assertEquals(accessControlList.grantList.get(0).grantee.canonicalId, "Grantee_ID");

    }

    @Test
    public void loadxml_GrantPermission_Test() {
        AccessControlList accessControlList = accessControlPolicy.getAccessControlList();
        assertEquals(accessControlList.grantList.get(0).permission, "Permission");
    }


    @Test
    public void loadxml_GetGranteeSize_Test() {
        AccessControlList accessControlList = accessControlPolicy.getAccessControlList();
        assertEquals(accessControlList.grantList.size(), 1);
    }

}


