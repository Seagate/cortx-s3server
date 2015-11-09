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

#include "s3_server_config.h"

S3Config* S3Config::instance = NULL;

S3Config::S3Config() {
  // Read config items from some file.
  default_endpoint = "s3.seagate.com";  // Default region endpoint (US)
  // TODO - load region endpoints from config
  std::string eps[] = {"s3-us.seagate.com", "s3-europe.seagate.com", "s3-asia.seagate.com"};
  for( int i = 0; i < 3; i++) {
    region_endpoints.insert(eps[i]);
  }
}

S3Config* S3Config::get_instance() {
  if(!instance){
    instance = new S3Config();
  }
  return instance;
}

std::string& S3Config::get_default_endpoint() {
    return default_endpoint;
}

std::set<std::string>& S3Config::get_region_endpoints() {
    return region_endpoints;
}
