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

###########################################################################
#
# This script runs in every s3 IO container right before entry-point
# script, so use it for whatever ad-hoc fixes needed in container.
#
# Make sure it's indempotent, i.e. repeated runs do not create
# problems (since k8s will be re-starting containers).
#
###########################################################################

s3_repo_dir=/var/data/cortx/cortx-s3server
src_dir="$s3_repo_dir"/scripts/env/kubernetes

# FIXME: motr/provisioner dependency on machine-id file
cat /etc/cortx/s3/machine-id > /etc/machine-id

# FIXME: use hard-coded cortx-utils version; newer ones have some unknown changes
rpm -e --nodeps cortx-py-utils
yum install -y "$src_dir"/message-bus/cortx-py-utils-2.0.0-423_411caf9.noarch.rpm
