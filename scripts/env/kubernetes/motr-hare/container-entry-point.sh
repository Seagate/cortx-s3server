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

set -euo pipefail # exit on failures
set -x

sync_file_path=/var/data/cortx/motr-hare-is-up.txt
  # This file will be created after Motr is up and rummings -- so that other
  # containers can know when motr is ready.
rm -f "$sync_file_path"

#echo "211072f61c4b4949839c624d6ed95115" > /etc/machine-id
cat /etc/cortx/s3/machine-id > /etc/machine-id
sed -i '103,107 s/^/#/'                                  /usr/libexec/cortx-motr/motr-server
sed -i '243,243 s/^/#/'                                  /usr/libexec/cortx-motr/motr-server
sed -i '209 i fi'                                        /usr/libexec/cortx-motr/motr-server
sed -i '209 i stob_type="-T AD"'                         /usr/libexec/cortx-motr/motr-server
sed -i '209 i if [[ "$svc_tag" = "0xa" ]] ; then'        /usr/libexec/cortx-motr/motr-server
sed -i '209 i svc_tag=$(echo $service | cut -d ":" -f2)' /usr/libexec/cortx-motr/motr-server

mkdir /etc/motr

src_dir=/var/data/cortx/cortx-s3server/scripts/env/kubernetes/motr-hare/

cp "$src_dir"/confd.xc                    /etc/motr/confd.xc
cp "$src_dir"/m0d-0x7200000000000001-0x1a /etc/sysconfig/m0d-0x7200000000000001:0x1a
cp "$src_dir"/m0d-0x7200000000000001-0xa  /etc/sysconfig/m0d-0x7200000000000001:0xa
cp "$src_dir"/m0d-0x7200000000000001-0xd  /etc/sysconfig/m0d-0x7200000000000001:0xd
cp "$src_dir"/m0d-0x7200000000000001-0x7  /etc/sysconfig/m0d-0x7200000000000001:0x7
cp "$src_dir"/motr-deploy                 /root/motr-deploy

chmod +x /root/motr-deploy

m0setup -Mv -p "/dev/sd[b-m]" --no-m0t1fs || true
  # ignoring error code, it's expected to fail due to D-Bus issue
sleep 5
/root/motr-deploy -s 0x7200000000000001:0x7 &>/var/log/cortx/motr-0x7.log &
sleep 5

/root/motr-deploy -s 0x7200000000000001:0xa &>/var/log/cortx/motr-0xa.log &

sleep 5

/root/motr-deploy -s 0x7200000000000001:0xd &>/var/log/cortx/motr-0xd.log &
  # Note: (confd) this service will throw rc = -111 error after start
  # and this errors will disappear once below command ioservice is started:

sleep 5

/root/motr-deploy -s 0x7200000000000001:0x1a &>/var/log/cortx/motr-0x1a.log &

sleep 5

if [ "`ps ax | grep /usr/bin/m0d | { grep -v grep || true } | wc -l`" -ne 4 ]; then
  echo
  ps ax
  echo
  tail -n4 /var/log/cortx/motr-0x{7,a,d,1a}.log
  echo
  echo FAILED.  Motr does not seem to be running properly.
  exit 1
fi

date > "$sync_file_path"

sleep infinity
