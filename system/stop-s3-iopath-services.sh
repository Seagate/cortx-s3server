#!/bin/bash
#
# Copyright (c) 2020 Seagate Technology LLC and/or its Affiliates
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

# Haproxy, S3authserver  and S3 server stop script in deployment environment.
#   Usage:stop-s3-iopath-services.sh 
for i in $(ls -d  /etc/sysconfig/s3server*)
do
  s3server_fid=${i%%/}
  result=$(basename "$s3server_fid")
  systemctl is-active --quiet s3server@"${result:9}" && systemctl stop s3server@"${result:9}"
done
systemctl stop s3authserver
systemctl stop haproxy
