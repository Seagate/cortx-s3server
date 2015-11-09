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

#pragma once

#ifndef __MERO_FE_S3_SERVER_S3_CLOVIS_CONFIG_H__
#define __MERO_FE_S3_SERVER_S3_CLOVIS_CONFIG_H__

#include <cstddef>

// We should load this from config file.
// Not thread-safe, but we are single threaded.
class S3ClovisConfig {
private:
  static S3ClovisConfig* instance;
  S3ClovisConfig();
  // S3ClovisConfig(std::string config_file);

  // Config items.
  size_t clovis_block_size;

  size_t clovis_idx_fetch_count;

public:
  static S3ClovisConfig* get_instance();

  // Config items
  size_t get_clovis_block_size();

  // Number of rows to fetch from clovis idx in single call
  size_t get_clovis_idx_fetch_count();
};

#endif
