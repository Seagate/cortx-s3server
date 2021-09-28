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
source ./sh/045-prepare-s3-containers-configs.sh

# Update producer endpoint
sed -i -e "s,producer_endpoint:.*,producer_endpoint: '"http://$CORTX_IO_SVC:28049"'," /etc/cortx/s3/s3backgrounddelete/config.yaml

add_separator Creating BG POD.

# update image link for bgdelete-producer pod
cat k8s-blueprints/s3-bg-producer-pod.yaml.template \
| sed "s,<s3-cortx-all-image>,ghcr.io/seagate/cortx-all:${S3_CORTX_ALL_IMAGE_TAG}," \
> k8s-blueprints/s3-bg-producer-pod.yaml

# pull the images
pull_images_for_pod k8s-blueprints/s3-bg-producer-pod.yaml

kubectl apply -f k8s-blueprints/s3-bg-producer-pod.yaml

set +x
while [ `kubectl get pod | grep s3-bg-producer-pod | grep Running | wc -l` -lt 1 ]; do
  echo
  kubectl get pod | grep 'NAME\|s3-bg-producer-pod'
  echo
  echo s3-bg-producer-pod is not yet in Running state, re-checking ...
  echo '(hit CTRL-C if it is taking too long)'
  sleep 5
done
set -x

add_separator SUCCESSFULLY CREATED BG POD.
