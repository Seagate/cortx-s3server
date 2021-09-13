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

source ./config.sh
source ./sh/functions.sh

set -e # exit immediatly on errors
set -x # print each statement before execution

> ./env.sh

hostname=`hostname`
set +x
echo
read -p "Input node FQDN (or hit Enter if default value is correct) [$hostname]:" var
if [ -n "$var" ]; then
  hostname="$var"
fi
set -x

echo "HOST_FQDN=$hostname" >> ./env.sh

NUM_CPU=`lscpu | grep '^CPU(s):' | awk -F: '{ gsub(/ /, "", $2); print $2 }'`
RAM_SIZE=`free | grep ^Mem | awk '{ print $2 }'`
NUM_DATA_DRIVES=$[ `ls /dev/sd? -1 | wc -l` - 1 ]

if [ "$NUM_CPU" -lt 16 ]; then
  self_check "Number of CPU is $NUM_CPU, which is less than 16. Are you sure you want to proceed?"
fi
echo "NUM_CPU=$NUM_CPU" >> ./env.sh

if [ "$RAM_SIZE" -lt 16000000 ]; then
  self_check "RAM size is $RAM_SIZE, which is less than 16000000. Are you sure you want to proceed?"
fi
echo "RAM_SIZE=$RAM_SIZE" >> ./env.sh

if [ "$NUM_DATA_DRIVES" -lt 12 ]; then
  self_check "Number of data drives is $NUM_DATA_DRIVES, which is less than 12. Are you sure you want to proceed?"
fi
echo "NUM_DATA_DRIVES=$NUM_DATA_DRIVES" >> ./env.sh

