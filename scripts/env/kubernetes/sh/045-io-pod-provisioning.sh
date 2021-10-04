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

add_separator Creating configs for S3 containers.

s3_repo_dir=/var/data/cortx/cortx-s3server
src_dir="$s3_repo_dir"/scripts/env/kubernetes

###########################################################################
# Create SHIM-POD #
###################

# update image link for containers
cat k8s-blueprints/shim-pod.yaml.template \
  | sed "s,<s3-cortx-all-image>,ghcr.io/seagate/cortx-all:${S3_CORTX_ALL_IMAGE_TAG}," \
  | sed "s,<motr-cortx-all-image>,ghcr.io/seagate/cortx-all:${MOTR_CORTX_ALL_IMAGE_TAG}," \
  > k8s-blueprints/shim-pod.yaml

# download images using docker -- 'kubectl init' is not able to apply user
# credentials, and so is suffering from rate limits.
pull_images_for_pod k8s-blueprints/shim-pod.yaml

delete_pod_if_exists shim-pod

kubectl apply -f k8s-blueprints/shim-pod.yaml

wait_till_pod_is_Running  shim-pod

kube_run() {
  kubectl exec -i shim-pod -c shim -- "$@"
}


############################
# S3 mini provisioner call #
############################

if [ "$USE_PROVISIONING" = yes ]; then
  kube_run "$src_dir/s3server/shim-provisioner.sh" io-pod
else
  kube_run "$src_dir/s3server/s3-mini-prov.sh"
fi

# #############
# # S3 server #
# #############

# 'manual' step for machine-id (until proper solution is merged) FIXME
kube_run sh -c 'cat /etc/machine-id > /etc/cortx/s3/machine-id-with-dashes'
kube_run sh -c 'cat /etc/machine-id | sed "s,-,,g" > /etc/cortx/s3/machine-id'

# Increase retry interval
sed -i \
  -e 's/S3_SERVER_BGDELETE_BIND_ADDR *:.*/S3_SERVER_BGDELETE_BIND_ADDR: 0.0.0.0/' \
  -e 's/S3_MOTR_RECONNECT_RETRY_COUNT *:.*/S3_MOTR_RECONNECT_RETRY_COUNT: 30/' \
  /etc/cortx/s3/conf/s3config.yaml


# ##################
# # S3 BG Services #
# ##################

set_bg_config_param() {
  key="$1"
  val="$2"
  sed -i -e "s,$key:.*,$key: $val," /etc/cortx/s3/s3backgrounddelete/config.yaml
}

# adjust delay delete parameters to speed up testing
set_bg_config_param  scheduler_schedule_interval      60
set_bg_config_param  leak_processing_delay_in_mins     1
set_bg_config_param  version_processing_delay_in_mins  1
set_bg_config_param  consumer_sleep  1
set_bg_config_param  purge_sleep     1


###########################################################################
# Destroy SHIM POD #
####################

# kubectl delete -f k8s-blueprints/shim-pod.yaml

add_separator SUCCESSFULLY CREATED CONFIGS FOR S3 CONTAINERS.
