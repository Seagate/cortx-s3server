/*
 * COPYRIGHT 2020 SEAGATE LLC
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
 * Original author:  Evgeniy Brazhnikov   <evgeniy.brazhnikov@seagate.com>
 * Author:  Evgeniy Brazhnikov   <evgeniy.brazhnikov@seagate.com>
 * Original creation date: 14-Jul-2020
 */

#pragma once

#ifndef __S3_SERVER_AUDIT_LOGGER_KAFKA_BRIDGE__
#define __S3_SERVER_AUDIT_LOGGER_KAFKA_BRIDGE__

#include <string>
#include <memory>

#include "s3_audit_info_logger_base.h"

class S3HttpPostQueue;

class S3AuditInfoLoggerKafkaBridge : public S3AuditInfoLoggerBase {
  typedef S3AuditInfoLoggerBase Base;
  typedef struct event_base evbase_t;

 public:
  S3AuditInfoLoggerKafkaBridge(evbase_t* p_base, std::string host,
                               uint16_t port, std::string path);
  ~S3AuditInfoLoggerKafkaBridge();

  S3AuditInfoLoggerKafkaBridge(const S3AuditInfoLoggerKafkaBridge&) = delete;
  S3AuditInfoLoggerKafkaBridge& operator=(const S3AuditInfoLoggerKafkaBridge&) =
      delete;

  int save_msg(std::string const&, std::string const&) override;

 private:
  std::unique_ptr<S3HttpPostQueue> p_s3_post_queue;
};

#endif  // __S3_SERVER_AUDIT_LOGGER_KAFKA_BRIDGE__
