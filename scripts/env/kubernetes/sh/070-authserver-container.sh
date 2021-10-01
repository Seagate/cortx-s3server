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

add_separator CONFIGURING AUTHSERVER CONTAINER.

kube_run() {
  kubectl exec -i io-pod -c authserver -- "$@"
}

#kube_run sh -c '/opt/seagate/cortx/auth/startauth.sh /etc/cortx/s3 &>/root/authserver.log &'

sleep 1

set +x
if [ -z "`kube_run ps ax | safe_grep 'java -jar AuthServer'`" ]; then
  echo
  kube_run ps ax
  echo
  add_separator FAILED.  AuthServer does not seem to be running.
  false
fi
set -x

add_separator SUCCESSFULLY CONFIGURED AUTHSERVER CONTAINER.
