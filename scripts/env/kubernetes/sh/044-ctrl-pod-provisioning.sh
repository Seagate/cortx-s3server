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

add_separator Running CTRL-POD provisioning.

s3_repo_dir=/var/data/cortx/cortx-s3server
src_dir="$s3_repo_dir"/scripts/env/kubernetes

###########################################################################
# Create SHIM-POD #
###################

# update image link for containers
replace_tags_and_create_pod  k8s-blueprints/shim-ctrl-pod.yaml.template  shim-ctrl-pod

kube_run() {
  kubectl exec -i shim-ctrl-pod -c shim -- "$@"
}


############################
# S3 mini provisioner call #
############################

kube_run "$src_dir/s3server/shim-provisioner.sh" ctrl-pod

# update solution config with proper values
#  python3 <<EOF
#import sys
#import yaml
#
#data = yaml.load(open("$BASE_CONFIG_PATH/s3/solution.cpy/config.yaml"))
#
#data['cortx']['external']['kafka']['endpoints'] = ['$KAFKA_EXTERNAL_IP']
#
#data['cortx']['common']['security']['domain_certificate'] = '/opt/seagate/cortx/s3/install/haproxy/ssl/s3.seagate.com.pem'
#data['cortx']['common']['security']['device_certificate'] = '/opt/seagate/cortx/s3/install/haproxy/ssl/s3.seagate.com.pem'
#
#data['cortx']['s3']['internal']['endpoints'] = 'http://cortx-io-svc:28049'
#
#with open("$BASE_CONFIG_PATH/s3/solution.cpy/config.yaml", 'w') as f:
#  f.write(yaml.dump(data))
#EOF


###########################################################################
# Destroy SHIM POD #
####################

# kubectl delete -f k8s-blueprints/shim-ctrl-pod.yaml

add_separator SUCCESSFULLY COMPLETED CTRL-POD PROVISIONING.
