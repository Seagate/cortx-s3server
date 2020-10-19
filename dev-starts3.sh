#!/bin/sh -e
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

# Script to start S3 server in dev environment.
#   Usage: sudo ./dev-starts3.sh [<Number of S3 sever instances>] [--fake_obj] [--fake_kvs | --redis_kvs] [--callgraph /path/to/graph]
#               Optional argument is:
#                   Number of S3 server instances to start.
#                   Max number of instances allowed = 20
#                   Default number of instances = 1

MAX_S3_INSTANCES_NUM=20

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
fake_obj=0
fake_kvs=0
redis_kvs=0

callgraph_mode=0
callgraph_out="/tmp/callgraph.out"

if [[ $1 =~ ^[0-9]+$ ]] && [ $1 -le $MAX_S3_INSTANCES_NUM ]
then
    num_instances=$1
    shift
fi

while [ "$1" != "" ]; do
    case "$1" in
        --fake_obj ) fake_obj=1;
                     echo "Stubs for motr object read/write ops";
                     ;;
        --fake_kvs ) fake_kvs=1;
                     echo "Stubs for motr kvs put/get/delete/create idx/remove idx";
                     ;;
        --redis_kvs ) redis_kvs=1;
                      echo "Redis based stubs for motr kvs put/get/delete";
                      ;;
        --callgraph ) callgraph_mode=1;
                      num_instances=1;
                      echo "Generate call graph with valgrind";
                      shift;
                      if ! [[ $1 =~ ^[[:space:]]*$ ]]
                      then
                          callgraph_out="$1";
                      fi
                      ;;
        * )
            echo "Invalid argument passed";
            exit 1
            ;;
    esac
    shift
done

if [ $fake_kvs == 1 ] && [ $redis_kvs == 1 ]; then
    echo "Only fake kvs or redis kvs can be specified";
    exit 1;
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

#Set the core file size to unlimited
ulimit -c unlimited

#Set max open file limit to 10240
ulimit -n 10240

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

s3_config_file="/opt/seagate/cortx/s3/conf/s3config.yaml"

# Ensure default working dir is present
s3_working_dir=`python -c '
import yaml;
print yaml.load(open("'$s3_config_file'"))["S3_SERVER_CONFIG"]["S3_DAEMON_WORKING_DIR"];
' | tr -d '\r\n'
`"/s3server-0x7200000000000000:0"

mkdir -p $s3_working_dir

# Log dir configured in s3config.yaml
s3_log_dir=`python -c '
import yaml;
print yaml.load(open("'$s3_config_file'"))["S3_SERVER_CONFIG"]["S3_LOG_DIR"];
' | tr -d '\r\n'
`"/s3server-0x7200000000000000:0"
mkdir -p $s3_log_dir

# s3 port configured in s3config.yaml
s3_port_from_config=`python -c '
import yaml;
print yaml.load(open("'$s3_config_file'"))["S3_SERVER_CONFIG"]["S3_SERVER_BIND_PORT"];
' | tr -d '\r\n'`

# Start the s3server
export PATH=$PATH:/opt/seagate/cortx/s3/bin
counter=0

# s3server cmd parameters allowing to fake some motr functionality
# --fake_motr_writeobj - stub for motr write object with all zeros
# --fake_motr_readobj - stub for motr read object with all zeros
# --fake_motr_createidx - stub for motr create idx - does nothing
# --fake_motr_deleteidx - stub for motr delete idx - does nothing
# --fake_motr_getkv - stub for motr get key-value - read from memory hash map
# --fake_motr_putkv - stub for motr put kye-value - stores in memory hash map
# --fake_motr_deletekv - stub for motr delete key-value - deletes from memory hash map
# for proper KV mocking one should use following combination
#    --fake_motr_createidx true --fake_motr_deleteidx true --fake_motr_getkv true --fake_motr_putkv true --fake_motr_deletekv true

fake_params=""
if [ $fake_kvs -eq 1 ]
then
    fake_params+=" --fake_motr_createidx true --fake_motr_deleteidx true --fake_motr_getkv true --fake_motr_putkv true --fake_motr_deletekv true"
fi

if [ $redis_kvs -eq 1 ]
then
    fake_params+=" --fake_motr_createidx true --fake_motr_deleteidx true --fake_motr_redis_kvs true"
fi

if [ $fake_obj -eq 1 ]
then
    fake_params+=" --fake_motr_writeobj true --fake_motr_readobj true"
fi

callgraph_cmd=""
if [ $callgraph_mode -eq 1 ]
then
    callgraph_cmd="valgrind -q --tool=callgrind --collect-jumps=yes --collect-systime=yes --callgrind-out-file=$callgraph_out"
fi

while [[ $counter -lt $num_instances ]]
do
  motr_local_port=`expr 101 + $counter`
  s3port=`expr $s3_port_from_config + $counter`
  pid_filename='/var/run/s3server.'$s3port'.pid'
  $callgraph_cmd s3server --s3pidfile $pid_filename \
           --motrlocal $local_ep:${motr_local_port} --motrha $ha_ep \
           --s3port $s3port --fault_injection true $fake_params --loading_indicators --getoid true
  ((++counter))
done
