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

#pragma once

#ifndef __S3_SERVER_S3_CLI_OPTIONS_H__
#define __S3_SERVER_S3_CLI_OPTIONS_H__

#include <gflags/gflags.h>
#include <glog/logging.h>

DECLARE_string(s3config);
DECLARE_string(s3layoutmap);

DECLARE_string(s3hostv4);
DECLARE_string(s3hostv6);
DECLARE_string(motrhttpapihost);
DECLARE_int32(s3port);
DECLARE_int32(motrhttpapiport);
DECLARE_string(s3pidfile);

DECLARE_string(s3loglevel);
DECLARE_string(audit_config);
DECLARE_string(audit_log_dir);

DECLARE_bool(perfenable);
DECLARE_string(perflogfile);

DECLARE_string(motrlocal);
DECLARE_string(motrha);
DECLARE_int32(motrlayoutid);
DECLARE_string(motrprofilefid);
DECLARE_string(motrprocessfid);

DECLARE_string(authhost);
DECLARE_int32(authport);
DECLARE_bool(disable_auth);

DECLARE_bool(fake_authenticate);
DECLARE_bool(fake_authorization);

DECLARE_bool(fake_motr_createobj);
DECLARE_bool(fake_motr_writeobj);
DECLARE_bool(fake_motr_readobj);
DECLARE_bool(fake_motr_deleteobj);
DECLARE_bool(fake_motr_createidx);
DECLARE_bool(fake_motr_deleteidx);
DECLARE_bool(fake_motr_getkv);
DECLARE_bool(fake_motr_putkv);
DECLARE_bool(fake_motr_deletekv);
DECLARE_bool(fake_motr_redis_kvs);
DECLARE_bool(fault_injection);
DECLARE_bool(reuseport);
DECLARE_bool(getoid);
DECLARE_bool(loading_indicators);
DECLARE_bool(addb);

DECLARE_string(statsd_host);
DECLARE_int32(statsd_port);

// Loads config and also processes the command line options.
// options specified on cli overrides the option specified in config file.
int parse_and_load_config_options(int argc, char** argv);

// Cleanups related to options processing.
void finalize_cli_options();

#endif
