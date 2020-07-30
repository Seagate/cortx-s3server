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

#!/bin/sh
MAX_S3_INSTANCES_NUM=20

if [ $# -gt 1 ];
then
  echo "Invalid number of arguments passed to the script"
  echo "Usage: sudo ./iss3up.sh [<instance number>]"
  exit 1
fi

# s3 port configured in s3config.yaml
s3_port_from_config=`python -c '
import yaml;
print yaml.load(open("/opt/seagate/cortx/s3/conf/s3config.yaml"))["S3_SERVER_CONFIG"]["S3_SERVER_BIND_PORT"];
' | tr -d '\r\n'`

instance=1
if [ $# -eq 1 ]; then
  if [[ $1 =~ ^[0-9]+$ ]] && [ $1 -le $MAX_S3_INSTANCES_NUM ]; then
    instance=$1
  else
    echo "Invalid argument [$1], needs to be an integer <= $MAX_S3_INSTANCES_NUM"
    exit 1
  fi
fi

if [[ $instance -ne 0 ]]; then
  s3port=$(($s3_port_from_config + $instance - 1))
  s3_pid="$(ps aux | grep "/var/run/s3server.$s3port.pid" | grep -v grep)"
  if [[ "$s3_pid" != "" ]]; then
    s3_instance=$(netstat -an | grep $s3port | grep LISTEN)
    if [[ "$s3_instance" != "" ]]; then
      echo "s3 server is running and listening on $s3port"
    else
      echo "s3 server is stopped for port: $s3port"
      exit 1
    fi
  else
    echo "s3 server is stopped for port: $s3port"
    exit 1
  fi
fi
