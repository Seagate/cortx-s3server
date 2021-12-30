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

add_separator "Creating Message Bus POD"

# download images using docker -- 'kubectl init' is not able to apply user
# credentials, and so is suffering from rate limits.
pull_images_for_pod ./k8s-blueprints/zookeper.yaml
pull_images_for_pod ./k8s-blueprints/kafka.yaml

kubectl create -f ./k8s-blueprints/zookeper.yaml
kubectl create -f ./k8s-blueprints/kafka.yaml

set +x
while [ `kubectl get pod | grep 'zookeper\|kafka' | grep Running | wc -l` -lt 1 ]; do
  echo
  kubectl get pod | grep 'NAME\|zookeper\|kafka'
  echo
  echo Zookeper/Kafka pod is not yet in Running state, re-checking ...
  echo '(hit CTRL-C if it is taking too long)'
  sleep 5
done
set -x

set +x

add_separator SUCCESSFULLY CREATED OPENLDAP POD
