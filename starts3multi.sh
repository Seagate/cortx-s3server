#!/bin/sh -x
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


# Start the s3server
export PATH=$PATH:/opt/seagate/cortx/s3/bin

# Get local address
modprobe lnet
lctl network up &>> /dev/null
LOCAL_NID=`lctl list_nids | head -1`
LOCAL_EP=$LOCAL_NID:12345:44
HA_EP=$LOCAL_NID:12345:45:1
CONFD_EP=$LOCAL_NID:12345:44:101
PROF_OPT='<0x7000000000000001:0>'

motr_local_port=100

for i in {8080..8099}
do
  cmd="s3server --motrlocal $LOCAL_EP:${motr_local_port} --motrha $HA_EP --motrconfd $CONFD_EP --s3port ${i} --authhost 127.0.0.1 --authport 9085 --log_dir /var/log/seagate/s3 --s3loglevel INFO"
  echo $cmd
  eval $cmd &
  ((motr_local_port++))
done
