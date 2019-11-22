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
 * Original author:  Shalaka Dharap
 * Original creation date: 20-November-2019
*/

package com.seagates3.policy;

import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.UnsupportedEncodingException;
import java.lang.reflect.Type;
import java.util.HashSet;
import java.util.Set;

import com.google.gson.Gson;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import com.google.gson.reflect.TypeToken;

public
class Actions {

 private
  static final String S3_ACTIONS_FILE = "/S3Actions.json";

 private
  static final String IAM_ACTIONS_FILE = "/IAMActions.json";

 private
  static Set<String> bucketOperations = new HashSet<String>();
 private
  static Set<String> objectOperations = new HashSet<String>();

 public
  static void init(PolicyUtil.Services service)
      throws UnsupportedEncodingException {
    InputStream in = null;
    switch (service) {
      case S3:
        in = Actions.class.getResourceAsStream(S3_ACTIONS_FILE);
        break;

      case IAM:
        in = Actions.class.getResourceAsStream(IAM_ACTIONS_FILE);
        break;

      default:
        break;
    }
    InputStreamReader reader = new InputStreamReader(in, "UTF-8");

    Gson gson = new Gson();
    Type listType = new TypeToken<HashSet<String>>() {}
    .getType();
    JsonParser jsonParser = new JsonParser();
    JsonObject element = (JsonObject)jsonParser.parse(reader);
    bucketOperations = gson.fromJson(element.get("Bucket"), listType);
    objectOperations = gson.fromJson(element.get("Object"), listType);
  }
 public
  static Set<String> getBucketOperations() { return bucketOperations; }

 public
  static Set<String> getObjectOperations() { return objectOperations; }
}
