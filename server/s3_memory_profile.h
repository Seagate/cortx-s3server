/*
 * COPYRIGHT 2017 SEAGATE LLC
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
 * Original creation date: 6-Feb-2017
 */

#pragma once

#ifndef __S3_SERVER_S3_MEMORY_PROFILE_H__
#define __S3_SERVER_S3_MEMORY_PROFILE_H__

class S3MemoryProfile {
  size_t memory_per_get_request();

  size_t memory_per_put_request();

  size_t max_memory_per_request(); /* max(get, put) */
 public:
  // Returns true if we have enough memory in mempool to process
  // either get/put request
  virtual bool we_have_enough_memory_for_get_obj();
  virtual bool we_have_enough_memory_for_put_obj();
};

#endif
