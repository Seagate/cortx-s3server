#!/bin/bash
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

set -euo pipefail # exit on failures

source ./config.sh
source ./env.sh
source ./sh/functions.sh

set -x # print each statement before execution

add_separator CHECKING BG IS WORKING.

MACHINE_ID="$(cat /etc/cortx/s3/machine-id-with-dashes)"
consumer_log="/var/log/cortx/s3/$MACHINE_ID/s3backgrounddelete/object_recovery_processor.log"
producer_log='/var/log/cortx/s3/s3backgrounddelete/object_recovery_scheduler.log'
while [ 0 -eq "$(cat "$consumer_log" | safe_grep 'Leak entry' | safe_grep 'processed successfully and deleted' | wc -l)" ]; do
  set -x
  add_separator 'Checking BG status...'
  tail -n10 "$producer_log" "$consumer_log"
  echo
  echo 'BG services have not yet triggered deletion, re-checking ...'
  echo '(hit CTRL-C if it is taking too long; should succeed within 2 minutes)'
  echo
  date
  sleep 15
done

add_separator SUCCESSFULLY CONFIRMED BG IS WORKING.
