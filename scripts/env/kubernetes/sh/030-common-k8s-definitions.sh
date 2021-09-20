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

add_separator "Creating common k8s definitions"

kubectl apply -f k8s-blueprints/storage-class.yaml

mkdir -p /var/motr
mkdir -p /etc/cortx /var/log/cortx /var/data/cortx

kubectl apply -f k8s-blueprints/motr-pv.yaml
kubectl apply -f k8s-blueprints/motr-pvc.yaml
kubectl apply -f k8s-blueprints/var-motr-pv.yaml
kubectl apply -f k8s-blueprints/var-motr-pvc.yaml
kubectl apply -f k8s-blueprints/s3-pv.yaml
kubectl apply -f k8s-blueprints/s3-pvc.yaml

set +x

add_separator Waiting for PVs to become Bound

while [ -n "`kubectl get pv | grep -v ^NAME | grep -v Bound`" ]; do
  echo
  kubectl get pv | grep -v Bound
  echo
  echo Some PVs are not yet Bound, re-checking ...
  echo '(hit CTRL-C if it is taking too long)'
  sleep 2
done

add_separator Waiting for PVCs to become Bound
while [ -n "`kubectl get pvc | grep -v ^NAME | grep -v Bound`" ]; do
  echo
  kubectl get pvc | grep -v Bound
  echo
  echo Some PVs are not yet Bound, re-checking ...
  echo '(hit CTRL-C if it is taking too long)'
  sleep 2
done

set -x

set +x

add_separator SUCCESSFULLY CREATED COMMON K8S DEFINITIONS
