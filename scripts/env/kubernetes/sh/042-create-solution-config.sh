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

add_separator Creating solution configs from provisioner repo.

if [ ! -d 'cortx-prvsnr' ]; then
  # git clone https://github.com/Seagate/cortx-prvsnr -b kubernetes
  git clone -b br/sachit/share-var-log-branch https://github.com/sachitanands/cortx-prvsnr-Kubernetes cortx-prvsnr
else
  cd cortx-prvsnr
  git pull --ff-only
  cd -
fi

mkdir -p /etc/cortx/s3/solution.cpy/
cp cortx-prvsnr/test/deploy/kubernetes/solution-config/* /etc/cortx/s3/solution.cpy/
  # FIXME: Ujjwal said all must use cortx-prvsnr/conf/*.template files, not this test folder

#kubectl apply -f /etc/cortx/s3/solution.cpy/secrets.yaml
kubectl apply -f k8s-blueprints/secrets.yaml
  # FIXME: using local copy until it's not merged upstream

add_separator SUCCESSFULLY CREATED SOLUTION CONFIG TEMPLATE.
