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

set -e # exit immediatly on errors

source ./config.sh
source ./env.sh
source ./sh/functions.sh

set -x # print each statement before execution

add_separator Creating IO POD and containers.

sysctl -w vm.max_map_count=30000000

# update image link for containers
cat k8s-blueprints/cortx-io-pod.yaml.template \
  | sed "s,<s3-cortx-all-image>,ghcr.io/seagate/cortx-all:${S3_CORTX_ALL_IMAGE_TAG}," \
  | sed "s,<motr-cortx-all-image>,ghcr.io/seagate/cortx-all:${MOTR_CORTX_ALL_IMAGE_TAG}," \
  > k8s-blueprints/cortx-io-pod.yaml

# pull the images
cat k8s-blueprints/cortx-io-pod.yaml | grep 'image:' | awk '{print $2}' | xargs -n1 docker pull

# create the POD and all whats needed
# moved to common k8s definitions
# kubectl apply -f k8s-blueprints/motr-pv.yaml
# kubectl apply -f k8s-blueprints/motr-pvc.yaml
# kubectl apply -f k8s-blueprints/var-motr-pv.yaml
# kubectl apply -f k8s-blueprints/var-motr-pvc.yaml
# kubectl apply -f k8s-blueprints/s3-pv.yaml
# kubectl apply -f k8s-blueprints/s3-pvc.yaml
kubectl apply -f k8s-blueprints/cortx-io-pod.yaml

set +x
while [ `kubectl get pod | grep cortx-io-pod | grep Running | wc -l` -lt 1 ]; do
  echo
  kubectl get pod | grep 'NAME\|cortx-io-pod'
  echo
  echo cortx-io-pod is not yet in Running state, re-checking ...
  echo '(hit CTRL-C if it is taking too long)'
  sleep 5
done
set -x

add_separator SUCCESSFULLY CREATED IO POD AND CONTAINERS.
