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
 * Original author:  Rajesh Nambiar   <rajesh.nambiar@seagate.com>
 * Original author:  Kaustubh Deorukhkar   <kaustubh.deorukhkar@seagate.com>
 * Original creation date: 2-March-2016
 */

#pragma once

#ifndef __MERO_FE_S3_SERVER_LOG_H__
#define __MERO_FE_S3_SERVER_LOG_H__

#include <stdio.h>
#include <sys/time.h>

#define S3_LOG_FATAL 1
#define S3_LOG_ERROR 2
#define S3_LOG_WARN  3
#define S3_LOG_INFO  4
#define S3_LOG_DEBUG 5



extern const char *log_level_str[S3_LOG_DEBUG];

extern FILE *fp_log;

extern int s3log_level;

//++
// Log format:
//   Log level:[HH:MM:SS,MilliSec Month:Date:Year] "<filename>":[line#] <function name>: <Log description>
// Example:
//   DEBUG:[15:46:17,091 02:02:2016] "server/s3server.cc":[219]  main: S3 Server running...
// --

#define __S3QUOTE(x) # x
#define  _S3QUOTE(x)  __S3QUOTE(x)
#define s3_debug_strlen(x)     strlen(x)
#define s3_log(loglevel,fmt, ...)  \
        if(loglevel <= s3log_level) {  \
          struct timeval tval;  \
          gettimeofday(&tval,NULL);   \
          unsigned int nMillisec = tval.tv_usec / 1000;  \
          struct tm * dm = localtime(&tval.tv_sec);      \
          fprintf(fp_log, "%s:[%02d:%02d:%02d,%03d %02d:%02d:%04d] " _S3QUOTE(__FILE__) ":[" _S3QUOTE(__LINE__) "]  %s: " \
          fmt "\n", log_level_str[loglevel-1], dm->tm_hour, dm->tm_min, dm->tm_sec,nMillisec, dm->tm_mon, dm->tm_mday,dm->tm_year + 1900, __func__, ## __VA_ARGS__); \
          fflush(fp_log);  \
        }
#endif
