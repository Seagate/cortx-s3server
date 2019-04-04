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
 * Original creation date: 04-Apr-2019
 */

/*
 * This Class represent Xml Node as
 <AccessControlList>
  <Grant>
   <Grantee xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
      xsi:type="CanonicalUser">
    <ID>Grantee_ID</ID>
    <DisplayName>Grantee_Name</DisplayName>
   </Grantee>
   <Permission>Permission</Permission>
  </Grant>
 </AccessControlList>
 */


package com.seagates3.authorization;
import java.util.ArrayList;

public class AccessControlList {

    ArrayList<Grant> grantList = new ArrayList<Grant>();

    public AccessControlList() {
    }

    ArrayList<Grant> getGrantList() {
        return grantList;
    }

    void addGrant(Grant grant) {
        grantList.add(grant);
    }

    void setGrant(Grant grant) {
    }

    Grant getGrant(String CanonicalID) {
        for (int counter = 0; counter < grantList.size(); counter++) {
            if(grantList.get(counter).grantee.canonicalId == CanonicalID) {
                return grantList.get(counter);
            }
        }

        return null;
    }

}
