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

add_separator Creating prerequisites.

s3_repo_dir=/var/data/cortx/cortx-s3server
src_dir="$s3_repo_dir"/scripts/env/kubernetes

###############
# Message Bus #
###############

mkdir -p "$BASE_CONFIG_PATH"/utils/
cat message-bus/message_bus.conf.template | \
  sed -e "s/<kafka-external-ip>/$KAFKA_EXTERNAL_IP/" \
  > "$BASE_CONFIG_PATH"/utils/message_bus.conf

mkdir -p /share/var/log/cortx/utils/message_bus
chmod 755 /share/var/log/cortx/utils/*
chmod 755 /share/var/log/cortx/utils/message_bus
touch /share/var/log/cortx/utils/message_bus/message_bus.log
chmod 755 /share/var/log/cortx/utils/message_bus/message_bus.log

mkdir -p "$BASE_CONFIG_PATH"/utils/conf/
cp message-bus/cortx.conf "$BASE_CONFIG_PATH"
cp message-bus/cortx.conf "$BASE_CONFIG_PATH"/utils/conf/


add_separator SUCCESSFULLY CREATED PREREQUISITES.
