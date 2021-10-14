#!/bin/sh -x
#
# Copyright (c) 2021 Seagate Technology LLC and/or its Affiliates
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# For any questions about this software or licensing,
# please email opensource@seagate.com or cortx-questions@seagate.com.
#

set -e -x

if [ $# -ne 1 ]
then
  echo "Invalid number of arguments passed to the script"
  echo "Usage: starthaproxy.sh <haproxy config path>"
  exit 1
fi

base_config_path=$1
haproxy_log_path=$2

haproxy_cfg_file_path="$base_config_path/s3/haproxy.cfg"

#haproxy_config=$(s3confstore "yaml:///opt/seagate/cortx/s3/mini-prov/s3_prov_config.yaml" getkey --key="S3_HAPROXY_LOG_CONFIG_FILE")
#LOG_FILE=$(s3confstore "properties://$haproxy_config_path/$haproxy_config" getkey --key="LOG_FILE")
#echo "Haproxy logfile - $LOG_FILE"
##source "$log_file_source_path"
#if [ -z "$LOG_FILE" ]; then
#  echo 'LOG_FILE is not specified.'
#  exit 1
#fi

# Create log dir
mkdir -p "$(dirname "$haproxy_log_path")"

# Run the configured haproxy
/usr/sbin/haproxy -Ws -f $haproxy_cfg_file_path -p /run/haproxy.pid 1>>"$haproxy_log_path" 2>&1
