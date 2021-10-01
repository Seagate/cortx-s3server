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
set -x

s3_repo_dir=/var/data/cortx/cortx-s3server
src_dir="$s3_repo_dir"/scripts/env/kubernetes

source "$src_dir/env.sh"
source "$src_dir/sh/functions.sh"

if [ "$1" = io-pod ]; then
  node_name="storage-node1"
elif [ "$1" = ctrl-pod ]; then
  node_name="control-node"
else
  add_separator "Wrong usage for shim-provisioner.sh: 1st arg must be either io-pod or ctrl-pod"
  exit 1
fi

# FIXME: manual fix for known problem as of 2021-09-30 (remove disabling services)
sed -i -e '/self.disable_services(services_list)/ d' "/opt/seagate/cortx/s3/bin/configcmd.py"

rm -f /etc/cortx/cluster.conf

cortx_setup config apply -f yaml:///etc/cortx/s3/solution.cpy/cluster.yaml -c yaml:///etc/cortx/cluster.conf
cortx_setup config apply -f yaml:///etc/cortx/s3/solution.cpy/config.yaml  -c yaml:///etc/cortx/cluster.conf

yq -r '.node | map_values(.name) ' /etc/cortx/cluster.conf \
  | grep "$node_name" \
  | awk -F: '{print $1}' \
  | sed 's,[ "],,g' > /etc/machine-id

MACHINE_ID="$(cat /etc/machine-id)"
sysconfig_dir="/etc/cortx/s3/$MACHINE_ID/sysconfig/"
mkdir -p "$sysconfig_dir"
( set -exuo pipefail # print all commands and exit on failures
  for f in "$src_dir"/s3server/s3server-*; do
    source "$f"
    /usr/bin/cp -f "$f" "$sysconfig_dir"
    cat "$f" > "$sysconfig_dir/s3server-$MOTR_PROCESS_FID"
  done
)

cortx_setup cluster bootstrap -c yaml:///etc/cortx/cluster.conf
