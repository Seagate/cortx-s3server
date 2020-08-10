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

package com.seagates3.policy;

import static org.junit.Assert.assertTrue;
import static org.junit.Assert.assertFalse;

import org.junit.BeforeClass;
import org.junit.Test;

public
class PrincipalArnParserTest {

 private
  static ArnParser arnParser;

  @BeforeClass public static void setUpBeforeClass() throws Exception {
    arnParser = new PrincipalArnParser();
  }

  @Test public void test_isArnFormatValid_success_user() {
    String arn = "arn:aws:iam::KO87b1p0TKWa184S6xrINQ:user/u1";
    assertTrue(arnParser.isArnFormatValid(arn));
  }

  @Test public void test_isArnFormatValid_fail_noResource() {
    String arn = "arn:aws:iam::KO87b1p0TKWa184S6xrINQ:";
    assertFalse(arnParser.isArnFormatValid(arn));
  }

  @Test public void test_isArnFormatValid_success_rootUser() {
    String arn = "arn:aws:iam::KO87b1p0TKWa184S6xrINQ:root";
    assertTrue(arnParser.isArnFormatValid(arn));
  }

  @Test public void test_isArnFormatValid_success_username() {
    String arn = "arn:aws:iam::123456789012:user/user-name";
    assertTrue(arnParser.isArnFormatValid(arn));
  }

  @Test public void test_isArnFormatValid_success_role() {
    String arn = "arn:aws:iam::123456789012:role/role-name";
    assertTrue(arnParser.isArnFormatValid(arn));
  }

  @Test public void test_isArnFormatValid_fail_invalid_fullArn() {
    String arn = "arn:aws:iam:us-east:123456789012:role/role-name";
    assertFalse(arnParser.isArnFormatValid(arn));
  }

  @Test public void test_isArnFormatValid_fail_s3_arn() {
    String arn = "arn:aws:s3::123456789012:role/role-name";
    assertFalse(arnParser.isArnFormatValid(arn));
  }

  @Test public void test_isArnFormatValid_fail_invalidArn_emptyAccoountid() {
    String arn = "arn:aws:iam:::role/role-name";
    assertFalse(arnParser.isArnFormatValid(arn));
  }

  @Test public void test_isArnFormatValid_fail_invalidArn_emptyUser() {
    String arn = "arn:aws:iam:::";
    assertFalse(arnParser.isArnFormatValid(arn));
  }

  @Test public void test_isArnFormatValid_fail_randomString() {
    String arn = "qwerty";
    assertFalse(arnParser.isArnFormatValid(arn));
  }

  @Test public void test_isArnFormatValid_fail_emptyString() {
    String arn = "";
    assertFalse(arnParser.isArnFormatValid(arn));
  }

  @Test public void test_isArnFormatValid_fail_nullString() {
    String arn = null;
    assertFalse(arnParser.isArnFormatValid(arn));
  }
}
