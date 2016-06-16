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

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "s3_error_messages.h"
#include "s3_log.h"

// Some declarations from s3server that are required to get compiled.
// TODO - Remove such globals by implementing config file.
// S3 Auth service
const char *auth_ip_addr = "127.0.0.1";
uint16_t auth_port = 8095;

const char *log_level_str[S3_LOG_DEBUG] = {"FATAL", "ERROR", "WARN", "INFO", "DEBUG"};

FILE *fp_log;

//To be read from config file
int s3log_level = S3_LOG_FATAL;


int main(int argc, char **argv) {
  // This will be taken care during S3 Config changes -- TODO
  fp_log = std::fopen("s3ut.log", "w");
  if(fp_log == NULL) {
    printf("Failed to open log file\n");
    return -1;
  }
  ::testing::InitGoogleTest(&argc, argv);
  ::testing::InitGoogleMock(&argc, argv);

  S3ErrorMessages::init_messages("resources/s3_error_messages.json");

  return RUN_ALL_TESTS();
}
