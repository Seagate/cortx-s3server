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

source "$src_dir/config.sh"
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

rm -f "$BASE_CONFIG_PATH"/cluster.conf

# FIXME: use hard-coded cortx-utils version
rpm -e --nodeps cortx-py-utils
#yum install -y http://cortx-storage.colo.seagate.com/releases/cortx/github/integration-custom-ci/centos-7.9.2009/custom-build-223/cortx_iso/cortx-py-utils-2.0.0-423_411caf9.noarch.rpm
yum install -y http://ssc-vm-g2-rhev4-0613.colo.seagate.com/ivan.tishchenko/public2/-/raw/main/saved-rpms/cortx-py-utils-2.0.0-423_411caf9.noarch.rpm

cortx_setup config apply -f yaml://"$BASE_CONFIG_PATH"/s3/solution.cpy/cluster.yaml -c yaml://"$BASE_CONFIG_PATH"/cluster.conf
cortx_setup config apply -f yaml://"$BASE_CONFIG_PATH"/s3/solution.cpy/config.yaml  -c yaml://"$BASE_CONFIG_PATH"/cluster.conf

# FIXME: motr/provisioner dependency on machine-id file
rm -f /etc/machine-id
yq -r '.node | map_values(.name) ' "$BASE_CONFIG_PATH"/cluster.conf \
  | grep "$node_name" \
  | awk -F: '{print $1}' \
  | sed 's,[ "],,g' > /etc/machine-id

# FIXME: copying FID files, till we ingtegrate HARE provisioner to automation
MACHINE_ID="$(cat /etc/machine-id)"
sysconfig_dir="$BASE_CONFIG_PATH/s3/sysconfig/$MACHINE_ID/"
mkdir -p "$sysconfig_dir"
( set -exuo pipefail # print all commands and exit on failures
  for f in "$src_dir"/s3server/s3server-*; do
    source "$f"
    /usr/bin/cp -f "$f" "$sysconfig_dir"
    cat "$f" > "$sysconfig_dir/s3server-$MOTR_PROCESS_FID"
  done
)

# FIXME: hotfix for issue with libmotr that needs no-dashes, vs s3server that needs dashes
MACHINE_ID_NO_DASHES="$(cat /etc/machine-id | sed s,-,,g)"
if [ "$MACHINE_ID" != "$MACHINE_ID_NO_DASHES" ]; then
  tgt="$BASE_CONFIG_PATH/s3/sysconfig/$MACHINE_ID_NO_DASHES"
  ln -fs "$BASE_CONFIG_PATH/s3/sysconfig/$MACHINE_ID" "$tgt"
fi

cortx_setup cluster bootstrap -c yaml://"$BASE_CONFIG_PATH"/cluster.conf
