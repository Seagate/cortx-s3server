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

#include "s3_clovis_config.h"

S3ClovisConfig* S3ClovisConfig::instance = NULL;

S3ClovisConfig::S3ClovisConfig() {
  // Read config items from some file.
  clovis_block_size = 4096;

  clovis_idx_fetch_count = 100;
}

S3ClovisConfig* S3ClovisConfig::get_instance() {
  if(!instance){
    instance = new S3ClovisConfig();
  }
  return instance;
}

size_t S3ClovisConfig::get_clovis_block_size() {
    return clovis_block_size;
}

size_t S3ClovisConfig::get_clovis_write_payload_size() {
    return clovis_block_size * 100;
}

size_t S3ClovisConfig::get_clovis_idx_fetch_count() {
  return clovis_idx_fetch_count;
}
