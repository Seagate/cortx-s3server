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

#!/bin/sh -e
# Script to start S3 server in dev environment.
#   Usage: sudo ./dev-starts3-noauth.sh [<Number of S3 sever instances>]
#               Optional argument is:
#                   Number of S3 server instances to start.
#                   Max number of instances allowed = 20
#                   Default number of instances = 1

MAX_S3_INSTANCES_NUM=20

if [ $# -gt 1 ]
then
  echo "Invalid number of arguments passed to the script"
  echo "Usage: sudo ./dev-starts3-noauth.sh [<Number of S3 server instances>]"
  exit 1
fi

set +e
tmp=$(systemctl status rsyslog)
res=$?
if [ "$res" -eq "4" ]
then
    echo "Rsyslog service not found. Exiting..."
    exit -1
fi
if [ "$res" -ne "0" ]
then
    echo "Starting Rsyslog..."
    tmp=$(systemctl start rsyslog)
    res=$?
    if [ "$res" -ne "0" ]
    then
        echo "Rsyslog service start failed. Exiting..."
        exit -1
    fi
fi
echo "rsyslog started"
set -e

num_instances=1

if [ $# -eq 1 ]
then
  if [[ $1 =~ ^[0-9]+$ ]] && [ $1 -le $MAX_S3_INSTANCES_NUM ]
  then
    num_instances=$1
  else
    echo "Invalid argument [$1], needs to be an integer <= $MAX_S3_INSTANCES_NUM"
    exit 1
  fi
fi

set -x

export LD_LIBRARY_PATH="$(pwd)/third_party/motr/motr/.libs/:"\
"$(pwd)/third_party/motr/helpers/.libs/:"\
"$(pwd)/third_party/motr/extra-libs/gf-complete/src/.libs/"

# Get local address
modprobe lnet
lctl network up &>> /dev/null
local_nid=`lctl list_nids | head -1`
local_ep=$local_nid:12345:33
ha_ep=$local_nid:12345:34:1

# Run m0dixinit
set +e
./third_party/motr/dix/utils/m0dixinit -l $local_nid:12345:34:100 -H $local_nid:12345:34:1 \
                 -p '<0x7000000000000001:0>' -I 'v|1:20' -d 'v|1:20' -a check 2> /dev/null \
                 | grep -E 'Metadata exists: false' > /dev/null
rc=$?
set -e
if [ $rc -eq 0 ]
then
./third_party/motr/dix/utils/m0dixinit -l $local_nid:12345:34:100 -H $local_nid:12345:34:1 \
                 -p '<0x7000000000000001:0>' -I 'v|1:20' -d 'v|1:20' -a create
fi

# Ensure default working dir is present
s3_working_dir="/var/seagate/s3/"
mkdir -p $s3_working_dir

s3_log_dir="/var/log/seagate/s3/"
mkdir -p $s3_log_dir

# Start the s3server
export PATH=$PATH:/opt/seagate/cortx/s3/bin
counter=1

while [[ $counter -le $num_instances ]]
do
  motr_local_port=`expr 100 + $counter`
  s3port=`expr 8080 + $counter`
  pid_filename='/var/run/s3server.'$s3port'.pid'
  echo "Starting S3Server in authentication disabled mode.."
  s3server --s3pidfile $pid_filename \
           --motrlocal $local_ep:${motr_local_port} --motrha $ha_ep \
           --s3port $s3port --fault_injection true --disable_auth true
  ((counter++))
done
