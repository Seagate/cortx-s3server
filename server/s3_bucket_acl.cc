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
 * Original author:  Kaustubh Deorukhkar   <kaustubh.deorukhkar@seagate.com>
 * Original creation date: 1-Oct-2015
 */

#include "s3_bucket_acl.h"

void S3BucketACL::set_owner_id(std::string id) {
  owner_id = id;
}

void S3BucketACL::set_owner_name(std::string name) {
  owner_name = name;
}

std::string S3BucketACL::to_json() {
  // TODO
  return "";
}

void S3BucketACL::from_json(std::string content) {
  // TODO
}

std::string& S3BucketACL::get_xml_response() {
  response_xml =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
         "<AccessControlPolicy xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">\n"
         "  <Owner>\n"
         "    <ID>" + owner_id + "</ID>\n"
         "      <DisplayName>" + owner_name + "</DisplayName>\n"
         "  </Owner>\n"
        //  TODO everything below is hardcoded till we support actual ACL
         "  <AccessControlList>\n"
         "    <Grant>\n"
         "      <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:type=\"Group\">\n"
         "        <URI>http://acs.seagate.com/groups/global/AuthenticatedUsers</URI>\n"
         "      </Grantee>\n"
         "      <Permission>READ</Permission>\n"
         "    </Grant>\n"
         "    <Grant>\n"
         "      <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:type=\"Group\">\n"
         "        <URI>http://acs.seagate.com/groups/global/AuthenticatedUsers</URI>"
         "      </Grantee>\n"
         "      <Permission>WRITE</Permission>\n"
         "    </Grant>\n"
         "    <Grant>\n"
         "      <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:type=\"Group\">\n"
         "        <URI>http://acs.seagate.com/groups/global/AuthenticatedUsers</URI>\n"
         "      </Grantee>\n"
         "      <Permission>READ_ACP</Permission>\n"
         "    </Grant>\n"
         "  </AccessControlList>\n"
         "</AccessControlPolicy>\n";

  return response_xml;
}
