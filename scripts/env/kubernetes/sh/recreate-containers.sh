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

set -e -x

kubectl delete -f k8s-blueprints/io-pod.yaml
kubectl apply -f k8s-blueprints/io-pod.yaml

set +x
while [ `kubectl get pod | grep io-pod | grep Running | wc -l` -lt 1 ]; do
  echo
  kubectl get pod | grep 'NAME\|io-pod'
  echo
  echo io-pod is not yet in Running state, re-checking ...
  echo '(hit CTRL-C if it is taking too long)'
  sleep 5
done
set -x

./sh/06-haproxy-container.sh
./sh/07-authserver-container.sh
./sh/08-motr-hare-container.sh
./sh/09-s3server-container.sh
