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

add_separator Creating BG POD.

# update image link for bgdelete-producer pod
cat k8s-blueprints/s3-bg-producer-pod.yaml.template \
| sed "s,<s3-cortx-all-image>,ghcr.io/seagate/cortx-all:${S3_CORTX_ALL_IMAGE_TAG}," \
> k8s-blueprints/s3-bg-producer-pod.yaml

# pull the images
pull_images_for_pod k8s-blueprints/s3-bg-producer-pod.yaml

delete_pod_if_exists "s3-bg-producer-pod"

kubectl apply -f k8s-blueprints/s3-bg-producer-pod.yaml

wait_till_pod_is_Running  s3-bg-producer-pod

add_separator SUCCESSFULLY CREATED BG POD.
