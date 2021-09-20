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

# Copy the SSL certificate file
if ! [ -f "/etc/cortx/s3/stx/stx.pem" ]; then
  if ! ( mkdir -p /etc/cortx/s3/stx/ && \
         cp /opt/seagate/cortx/s3/install/haproxy/ssl/s3.seagate.com.pem /etc/cortx/s3/stx/stx.pem ) \
  then
    echo "Failed to update SSL cert file /etc/cortx/s3/stx/stx.pem from /etc/ssl/stx/stx.pem."
    exit 1
  fi
fi

# Run the configured haproxy
mkdir -p /var/log/cortx/s3
/usr/sbin/haproxy -Ws -f /etc/cortx/s3/haproxy.cfg -p /run/haproxy.pid 1>>/var/log/cortx/s3/haproxy.log 2>&1
