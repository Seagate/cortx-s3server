#!/bin/sh
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

# Script to stop S3 server in dev environment.
# The script will stop all running instances of S3 server.

callgraph_process=0
callgraph_path="/tmp/callgraph.out"
if [ "$1" == "--callgraph" ]
then
    callgraph_process=1
    if ! [[ $2 =~ ^[[:space:]]*$ ]]
    then
        callgraph_path=$2
    fi
fi

MAX_S3_INSTANCES_NUM=20

# s3 port configured in s3config.yaml
s3_config_file="/opt/seagate/cortx/s3/conf/s3config.yaml"
s3_port_from_config=`python -c '
import yaml;
print yaml.load(open("'$s3_config_file'"))["S3_SERVER_CONFIG"]["S3_SERVER_BIND_PORT"];
' | tr -d '\r\n'`

instance=0
while [[ $instance -lt $MAX_S3_INSTANCES_NUM ]]
do
  s3port=$(($s3_port_from_config + $instance))
  pidfile="/var/run/s3server.$s3port.pid"
  if [[ -r $pidfile ]]; then
    pidstr=$(cat $pidfile)
    if [ "$pidstr" != "" ]; then
      kill -s TERM $pidstr
    fi # $pidstr
  fi # $pidfile

  ((instance++))
done # while
echo "Waiting for S3 to shutdown..."
sleep 10

instance=0
while [[ $instance -lt $MAX_S3_INSTANCES_NUM ]]
do
  s3port=$(($s3_port_from_config + $instance))
  statuss3=$(ps -aef | grep /var/run/s3server.$s3port.pid | grep $s3port)
  pidfile="/var/run/s3server.$s3port.pid"
  if [ "$statuss3" != "" ]; then
    if [[ -r $pidfile ]]; then
      pidstr=$(cat $pidfile)
      kill -9 $pidstr
    fi
  fi

  if [[ -e $pidfile ]]; then
    rm -f $pidfile
  fi

  ((instance++))
done

if [ $callgraph_process -eq 1 ]
then
    callgrind_annotate --inclusive=yes --tree=both "$callgraph_path" > "$callgraph_path".annotated
fi
