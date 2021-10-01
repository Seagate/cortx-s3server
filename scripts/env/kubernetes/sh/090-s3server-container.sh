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

add_separator Checking health of s3server container.

kube_run() {
  kubectl exec -i io-pod -c s3server-1 -- "$@"
}


add_separator Waiting for Motr to become available

while sleep 2; do
  if [ -f /var/data/cortx/motr-hare-is-up.txt ]; then
    break
  fi
  kubectl get pod io-pod
  ps ax | safe_grep m0d
done

add_separator Checking if S3 is responsive

while ! curl -I "http://$IO_POD_IP:28071"; do
  set +x
  echo
  echo "S3 is not yet listening on the port, re-trying..."
  echo "(hit CTRL-C if it's taking too long)"
  echo
  echo "Note: as of 2021-09-23: known issue clovis may freeze. work around --"
  echo "in separate console on the same VM - kill s3servers:"
  echo "ps ax | grep s3startsy | grep -v grep | awk '{print \$1}' | xargs kill -9"
  echo
  echo "NOTE -- kill -9 solution, you may need to wait for a minute till it works."
  echo
  set -x
  date
  sleep 2
done

add_separator S3SERVER CONTAINER IS HEALTHY.
