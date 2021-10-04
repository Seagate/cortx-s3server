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

mkdir -p /etc/cortx/utils/
cat message-bus/message_bus.conf.template | \
  sed -e "s/<kafka-external-ip>/$KAFKA_EXTERNAL_IP/" \
  > /etc/cortx/utils/message_bus.conf

mkdir -p /share/var/log/cortx/utils/message_bus
chmod 755 /share/var/log/cortx/utils/*
chmod 755 /share/var/log/cortx/utils/message_bus
touch /share/var/log/cortx/utils/message_bus/message_bus.log
chmod 755 /share/var/log/cortx/utils/message_bus/message_bus.log

mkdir -p /etc/cortx/utils/conf/
cp message-bus/cortx.conf /etc/cortx
cp message-bus/cortx.conf /etc/cortx/utils/conf/


# #############
# # S3 server #
# #############

# FIXME: create FID files manually (till hare prvsnr is ready)
mkdir -p /etc/cortx/s3/sysconfig
cat s3server/s3server-1 > '/etc/cortx/s3/sysconfig/s3server-0x7200000000000001:0x27'
cat s3server/s3server-2 > '/etc/cortx/s3/sysconfig/s3server-0x7200000000000001:0x2a'
cat s3server/s3server-3 > '/etc/cortx/s3/sysconfig/s3server-0x7200000000000001:0x2d'
cat s3server/s3server-4 > '/etc/cortx/s3/sysconfig/s3server-0x7200000000000001:0x30'
cat s3server/s3server-5 > '/etc/cortx/s3/sysconfig/s3server-0x7200000000000001:0x33'

# FIXME: 'manual' mapping of S3 FID files, until setupcmd.py fixes are ready to automate this
mkdir -p /etc/cortx/s3/sysconfig
cat s3server/s3server-1 > /etc/cortx/s3/sysconfig/s3server-1
cat s3server/s3server-2 > /etc/cortx/s3/sysconfig/s3server-2
cat s3server/s3server-3 > /etc/cortx/s3/sysconfig/s3server-3
cat s3server/s3server-4 > /etc/cortx/s3/sysconfig/s3server-4
cat s3server/s3server-5 > /etc/cortx/s3/sysconfig/s3server-5

add_separator SUCCESSFULLY CREATED PREREQUISITES.
