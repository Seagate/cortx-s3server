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

set -e # exit immediatly on errors

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
  kubectl exec -i io-pod -c hare-motr -- ps ax | grep m0d || true
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
  set -x
  sleep 2
done

# add_separator Checking if S3 is up
# 
# sleep 10
# 
# set +x
# if [ "`kube_run ps ax | grep 's3server --s3pidfile' | wc -l`" -ne 1 ]; then
#   echo
#   kubectl logs io-pod s3server-1
#   echo
#   kube_run ps ax
#   echo
#   add_separator FAILED.  S3 Server does not seem to be running properly.
#   false
# fi
# set -x

add_separator S3SERVER CONTAINER IS HEALTHY.
