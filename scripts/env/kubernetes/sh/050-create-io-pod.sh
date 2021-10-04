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

add_separator Creating IO POD and containers.

sysctl -w vm.max_map_count=30000000

# update image link for containers
cat k8s-blueprints/io-pod.yaml.template \
  | sed "s,<s3-cortx-all-image>,ghcr.io/seagate/cortx-all:${S3_CORTX_ALL_IMAGE_TAG}," \
  | sed "s,<motr-cortx-all-image>,ghcr.io/seagate/cortx-all:${MOTR_CORTX_ALL_IMAGE_TAG}," \
  > k8s-blueprints/io-pod.yaml

# FIXME: work-arounds for s3 containers
cp s3server/fix-container.sh /etc/cortx/s3

# pull the images
pull_images_for_pod k8s-blueprints/io-pod.yaml

delete_pod_if_exists io-pod

kubectl apply -f k8s-blueprints/io-pod.yaml

wait_till_pod_is_Running io-pod

set_var_POD_IP io-pod
echo "IO_POD_IP='$POD_IP'" >> env.sh

add_separator SUCCESSFULLY CREATED IO POD AND CONTAINERS.

add_separator Creating Service for IO Pod

# Creating endpoint for service
kubectl apply -f k8s-blueprints/cortx-io-ep.yaml
# Creating service
kubectl apply -f k8s-blueprints/cortx-io-svc.yaml

if [ `kubectl get svc | safe_grep cortx-io-svc | wc -l` -lt 1 ]
then
   add_separator FAILED. cortx io service does not seem to be running.
   exit 1
fi

set_var_SVC_IP cortx-io-svc
echo "CORTX_IO_SVC='$SVC_IP'" >> env.sh 

add_separator SUCCESSFULLY CREATED IO SERVICE.
