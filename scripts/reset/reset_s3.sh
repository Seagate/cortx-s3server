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


usage() {
  echo "Usage: $0 [--check-motr-cleanup][--cleanup-rabbitmq]"
  echo '          --check-motr-cleanup       : Checks if motr indexes are cleaned'
  echo '          --cleanup-rabbitmq         : Purges rabbitmq S3 queue'
  echo '          --help (-h)                : Display help'
}

SCRIPT_PATH=$(readlink -f "$0")
BASEDIR=$(dirname "$SCRIPT_PATH")

check_motr_cleanup=0
cleanup_rabbitmq=0

if [ $# -eq 0 ]
  then
    echo "Please provide valid input"
    usage
    exit 1
fi

while true; do
  case "$1" in
    --check-motr-cleanup) check_motr_cleanup=1; shift ;;
    --cleanup-rabbitmq ) cleanup_rabbitmq=1; shift ;;
    -h|--help) usage; exit 0;;
    *) break;;
  esac
done

# Sanity to check motr indexes are clean
# This check will fail if S3server is not running.
if [ $check_motr_cleanup -eq 1 ]
then
  python3.6 ${BASEDIR}/precheck.py
  if [ $? -ne 0 ]
  then
    echo "Motr indexes are not clean or all S3 instances are not running" >&2
    exit 1
  else
    echo "Motr indexes are cleaned and all S3 instances running"
  fi
fi

# Cleanup Rabbitmq entries from S3 queue
#  This check will fail if RabbitMQ is not running.
if [ $cleanup_rabbitmq -eq 1 ]
then
  systemctl status rabbitmq-server >/dev/null || { echo >&2 "Rabbitmq is not running."; exit 1; }
  command -v rabbitmqadmin >/dev/null || \
    { echo >&2 "rabbitmqadmin cli not found management plugin should be enabled"; exit 1; }

  echo "Cleaning rabbitmq entires"
  rabbitmqadmin purge queue name=s3_delete_obj_job_queue

  if [ $? -ne 0 ]
  then
    echo "S3 failed to purge Rabbitmq" >&2
    exit 1
  fi
fi

echo "S3 reset Success"
