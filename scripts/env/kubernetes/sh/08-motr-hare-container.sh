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

add_separator CONFIGURING MOTR-HARE CONTAINER.

kube_run() {
  kubectl exec -i depl-pod -c hare-motr -- "$@"
}

cat <<'EOF' | kube_run sh
set -x -e

echo "211072f61c4b4949839c624d6ed95115" > /etc/machine-id
sed -i '103,107 s/^/#/' /usr/libexec/cortx-motr/motr-server
sed -i '243,243 s/^/#/' /usr/libexec/cortx-motr/motr-server
sed -i '209 i fi' /usr/libexec/cortx-motr/motr-server
sed -i '209 i stob_type="-T AD"' /usr/libexec/cortx-motr/motr-server
sed -i '209 i if [[ "$svc_tag" = "0xc" ]] ; then' /usr/libexec/cortx-motr/motr-server
sed -i '209 i svc_tag=$(echo $service | cut -d ":" -f2)' /usr/libexec/cortx-motr/motr-server
mkdir /etc/motr
touch /root/motr-deploy
chmod +x /root/motr-deploy

EOF

cat motr-hare/confd.xc | kube_run tee /etc/motr/confd.xc
cat motr-hare/m0d-0x7200000000000001:0x6 | kube_run tee /etc/sysconfig/m0d-0x7200000000000001:0x6
cat motr-hare/m0d-0x7200000000000001:0x9 | kube_run tee /etc/sysconfig/m0d-0x7200000000000001:0x9
cat motr-hare/m0d-0x7200000000000001:0xc | kube_run tee /etc/sysconfig/m0d-0x7200000000000001:0xc
cat motr-hare/motr-deploy | kube_run tee /root/motr-deploy


cat <<'EOF' | kube_run sh
set -x -e

m0setup -Mv -p "/dev/sd[b-m]" --no-m0t1fs || true
  # ignoring error code, it's expected to fail due to D-Bus issue

sleep 5

/root/motr-deploy -s 0x7200000000000001:0x6 &>/root/motr-0x6.log &

sleep 5

/root/motr-deploy -s 0x7200000000000001:0x9 &>/root/motr-0x9.log &
  # Note: (confd) this service will throw rc = -111 error after start
  # and this errors will disappear once below command ioservice is started:

sleep 5

/root/motr-deploy -s 0x7200000000000001:0xc &>/root/motr-0xc.log &
EOF

sleep 5

set +x
if [ "`kube_run ps ax | grep /usr/bin/m0d | wc -l`" -ne 3 ]; then
  echo
  kube_run ps ax
  echo
  add_separator FAILED.  Motr does not seem to be running properly.
  false
fi
set -x

add_separator SUCCESSFULLY CONFIGURED MOTR-HARE CONTAINER.
