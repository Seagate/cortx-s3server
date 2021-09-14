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
source ./env.sh
source ./sh/functions.sh

set -e # exit immediatly on errors
set -x # print each statement before execution

add_separator CONFIGURING HAPROXY CONTAINER.

kube_run() {
  kubectl exec -i depl-pod -c haproxy -- "$@"
}

# Find haproxy version:
haproxy_ver=$( kube_run yum list installed | grep haproxy | awk '{print $2}' )

if [[ ! "$haproxy_ver" =~ ^2\.2\. ]]; then
  self_check "Haproxy version is <$haproxy_ver>; expected is 2.2.x.  Are you sure you want to continue?"
fi

kube_run mkdir -p /etc/cortx/s3/ \
                  /etc/cortx/s3/stx/ \
                  /etc/haproxy/errors

cat haproxy/haproxy.cfg | kube_run tee /etc/cortx/s3/haproxy.cfg
cat haproxy/503.http    | kube_run tee /etc/haproxy/errors/503.http
cat haproxy/stx.pem     | kube_run tee /etc/cortx/s3/stx/stx.pem

kube_run /bin/bash -c '/opt/seagate/cortx/s3/bin/s3_start --service haproxy &'

set +x
if [ -z "`kube_run ps ax | grep 'haproxy.pid'`" ]; then
  echo
  kube_run ps ax
  echo
  add_separator FAILED.  haproxy does not seem to be running.
  false
fi
set -x

add_separator SUCCESSFULLY CONFIGURED HAPROXY CONTAINER.
