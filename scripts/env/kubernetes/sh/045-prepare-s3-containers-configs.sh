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
cat k8s-blueprints/shim-pod.yaml | grep 'image:' | awk '{print $2}' | xargs -n1 docker pull

kubectl apply -f k8s-blueprints/shim-pod.yaml

set +x
while [ `kubectl get pod | grep shim-pod | grep Running | wc -l` -lt 1 ]; do
  echo
  kubectl get pod | grep 'NAME\|shim-pod'
  echo
  echo shim-pod is not yet in Running state, re-checking ...
  echo '(hit CTRL-C if it is taking too long)'
  sleep 5
done
set -x

kube_run() {
  kubectl exec -i shim-pod -c shim -- "$@"
}


##############
# machine ID #
##############
mkdir -p /etc/cortx/s3
echo "211072f61c4b4949839c624d6ed95115" > /etc/cortx/s3/machine-id


###########
# haproxy #
###########

mkdir -p /etc/cortx/s3/ \
         /etc/cortx/s3/stx/ \
         /etc/cortx/s3/haproxy/errors/

cp haproxy/haproxy.cfg /etc/cortx/s3/haproxy.cfg
cp haproxy/503.http    /etc/cortx/s3/haproxy/errors/503.http
cp haproxy/stx.pem     /etc/cortx/s3/stx/stx.pem


##############
# authserver #
##############

mkdir -p /etc/cortx/s3/auth/resources
cp -r "$s3_repo_dir"/auth/resources/* /etc/cortx/s3/auth/resources/

cp /etc/cortx/s3/auth/resources/authserver.properties.sample \
   /etc/cortx/s3/auth/resources/authserver.properties
cp /etc/cortx/s3/auth/resources/keystore.properties.sample \
   /etc/cortx/s3/auth/resources/keystore.properties

# Update needed properties in Auth config

# Generate encrypted ldap password for sgiam-admin on Pod

const_key=`kube_run s3cipher generate_key --const_key cortx`
encrypted_pwd=`kube_run  s3cipher encrypt --data "ldapadmin" --key "$const_key"`

set_var_OPENLDAP_SVC

# Copy encrypted ldap-passsword (ldapLoginPW) and openldap endpoint (key name
# ldapHost) to authserver.properties file.

sed -i "s|^ldapLoginPW *=.*|ldapLoginPW=$encrypted_pwd|;
        s|^ldapHost *=.*|ldapHost=$OPENLDAP_SVC|" \
    /etc/cortx/s3/auth/resources/authserver.properties

#############
# S3 server #
#############

mkdir -p /etc/cortx/s3/conf
cp "$s3_repo_dir"/s3config.release.yaml.sample \
   /etc/cortx/s3/conf/s3config.yaml
cat s3server/s3server-1 > /etc/cortx/s3/s3server-1
cat s3server/s3server-2 > /etc/cortx/s3/s3server-2
cat s3server/s3server-3 > /etc/cortx/s3/s3server-3
cat s3server/s3server-4 > /etc/cortx/s3/s3server-4
cat s3server/s3server-5 > /etc/cortx/s3/s3server-5


###########################################################################
# Destroy SHIM POD #
####################

kubectl delete -f k8s-blueprints/shim-pod.yaml

add_separator SUCCESSFULLY CREATED CONFIGS FOR S3 CONTAINERS.
