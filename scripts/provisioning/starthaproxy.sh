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

source /etc/cortx/s3/sysconfig/haproxy

if [ -z "$LOG_FILE" ]; then
  echo 'LOG_FILE is not specified.'
  exit 1
fi

# Create log dir
mkdir -p "$(dirname "$LOG_FILE")"

# Run the configured haproxy
/usr/sbin/haproxy -Ws -f /etc/cortx/s3/haproxy.cfg -p /run/haproxy.pid 1>>"$LOG_FILE" 2>&1
