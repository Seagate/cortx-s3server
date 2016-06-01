/*
 * COPYRIGHT 2016 SEAGATE LLC
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
 * Original author:  Rajesh Nambiar   <rajesh.nambiarr@seagate.com>
 * Original creation date: 02-June-2016
 */

#include "gtest/gtest.h"
#include "base64.h"
#include "s3_object_acl.h"

TEST(S3ObjectACLTest, Constructor) {
  S3ObjectACL object;
  EXPECT_EQ("", object.get_xml_str());
  EXPECT_EQ("", object.get_acl_metadata());
}

TEST(S3ObjectACLTest, SetOwnerName) {
  S3ObjectACL object;
  object.set_owner_name("s3server");
  object.get_owner_name();
}

TEST(S3ObjectACLTest, FromJson) {
  std::string acl_json = "{\"ACL\":\"PD94bWwgdmVyc2lvbj0iMS4wIiBlbmNvZGluZz0iVVRGLTgiPz4KPEFjY2Vzc0NvbnRyb2xQb2xpY3kgeG1sbnM9Imh0dHA6Ly9zMy5hbWF6b25hd3MuY29tL2RvYy8yMDA2LTAzLTAxLyI+CiAgPE93bmVyPgogICAgPElEPjEyMzQ1PC9JRD4KICA8L093bmVyPgogIDxBY2Nlc3NDb250cm9sTGlzdD4KICAgIDxHcmFudD4KICAgICAgPEdyYW50ZWUgeG1sbnM6eHNpPSJodHRwOi8vd3d3LnczLm9yZy8yMDAxL1hNTFNjaGVtYS1pbnN0YW5jZSIgeHNpOnR5cGU9IkNhbm9uaWNhbFVzZXIiPgogICAgICAgIDxJRD4xMjM0NTwvSUQ+CiAgICAgICAgPERpc3BsYXlOYW1lPnMzX3Rlc3Q8L0Rpc3BsYXlOYW1lPgogICAgICA8L0dyYW50ZWU+CiAgICAgIDxQZXJtaXNzaW9uPkZVTExfQ09OVFJPTDwvUGVybWlzc2lvbj4KICAgIDwvR3JhbnQ+CiAgPC9BY2Nlc3NDb250cm9sTGlzdD4KPC9BY2Nlc3NDb250cm9sUG9saWN5Pgo=\",\"Bucket-Name\":\"seagatebucket\"}";

  std::string acl_val = "PD94bWwgdmVyc2lvbj0iMS4wIiBlbmNvZGluZz0iVVRGLTgiPz4KPEFjY2Vzc0NvbnRyb2xQb2xpY3kgeG1sbnM9Imh0dHA6Ly9zMy5hbWF6b25hd3MuY29tL2RvYy8yMDA2LTAzLTAxLyI+CiAgPE93bmVyPgogICAgPElEPjEyMzQ1PC9JRD4KICA8L093bmVyPgogIDxBY2Nlc3NDb250cm9sTGlzdD4KICAgIDxHcmFudD4KICAgICAgPEdyYW50ZWUgeG1sbnM6eHNpPSJodHRwOi8vd3d3LnczLm9yZy8yMDAxL1hNTFNjaGVtYS1pbnN0YW5jZSIgeHNpOnR5cGU9IkNhbm9uaWNhbFVzZXIiPgogICAgICAgIDxJRD4xMjM0NTwvSUQ+CiAgICAgICAgPERpc3BsYXlOYW1lPnMzX3Rlc3Q8L0Rpc3BsYXlOYW1lPgogICAgICA8L0dyYW50ZWU+CiAgICAgIDxQZXJtaXNzaW9uPkZVTExfQ09OVFJPTDwvUGVybWlzc2lvbj4KICAgIDwvR3JhbnQ+CiAgPC9BY2Nlc3NDb250cm9sTGlzdD4KPC9BY2Nlc3NDb250cm9sUG9saWN5Pgo=";

  Json::Value newroot;
  Json::Reader reader;
  bool parsingSuccessful = reader.parse(acl_json, newroot);
  if(!parsingSuccessful) {
    printf("Reading of json failed\n");
  }
  S3ObjectACL object;
  object.from_json(newroot["ACL"].asString());
  EXPECT_EQ(acl_val, object.get_acl_metadata().c_str());
  EXPECT_EQ(base64_decode(acl_val), object.get_xml_str().c_str());
}
