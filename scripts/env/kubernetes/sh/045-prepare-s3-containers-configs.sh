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
pull_images_for_pod k8s-blueprints/shim-pod.yaml

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


###############
# Message Bus #
###############

mkdir -p /etc/cortx/utils/
cat message-bus/message_bus.conf.template | \
  sed -e "s/<kafka-external-ip>/$KAFKA_EXTERNAL_IP/" \
  > /etc/cortx/utils/message_bus.conf

mkdir -p /var/log/cortx/utils/message_bus
chmod 755 /var/log/cortx/utils/*
chmod 755 /var/log/cortx/utils/message_bus
touch /var/log/cortx/utils/message_bus/message_bus.log
chmod 755 /var/log/cortx/utils/message_bus/message_bus.log

cp message-bus/cortx.conf /etc/cortx


############################
# S3 mini provisioner call #
############################

kube_run "$src_dir/s3server/s3-mini-prov.sh"


# 'manual' step for machine-id (until proper solution is merged) FIXME
kube_run sh -c 'cat /etc/machine-id > /etc/cortx/s3/machine-id'

# ##############
# # machine ID #
# ##############
# mkdir -p /etc/cortx/s3
# echo "211072f61c4b4949839c624d6ed95115" > /etc/cortx/s3/machine-id
# 
# 
# ###########
# # haproxy #
# ###########
# 
# mkdir -p /etc/cortx/s3/ \
#          /etc/cortx/s3/stx/ \
#          /etc/cortx/s3/haproxy/errors/
# 
# cp haproxy/haproxy.cfg /etc/cortx/s3/haproxy.cfg
# cp haproxy/503.http    /etc/cortx/s3/haproxy/errors/503.http
# cp haproxy/stx.pem     /etc/cortx/s3/stx/stx.pem
# 
# 
# ##############
# # authserver #
# ##############
# 
# mkdir -p /etc/cortx/auth/resources
# cp -r "$s3_repo_dir"/auth/resources/* /etc/cortx/auth/resources/
# 
# cp /etc/cortx/auth/resources/authserver.properties.sample \
#    /etc/cortx/auth/resources/authserver.properties
# cp /etc/cortx/auth/resources/keystore.properties.sample \
#    /etc/cortx/auth/resources/keystore.properties
# 
# # Update needed properties in Auth config
# 
# # Generate encrypted ldap password for sgiam-admin on Pod
# 
# const_key=`kube_run s3cipher generate_key --const_key cortx`
# encrypted_pwd=`kube_run  s3cipher encrypt --data "ldapadmin" --key "$const_key"`
# 
# # Copy encrypted ldap-passsword (ldapLoginPW) and openldap endpoint (key name
# # ldapHost) to authserver.properties file.
# 
# sed -i "s|^ldapLoginPW *=.*|ldapLoginPW=$encrypted_pwd|;
#         s|^ldapHost *=.*|ldapHost=$OPENLDAP_SVC|" \
#     /etc/cortx/auth/resources/authserver.properties
# 
# #############
# # S3 server #
# #############
# 
# mkdir -p /etc/cortx/s3/conf
# cp "$s3_repo_dir"/s3config.release.yaml.sample \
#    /etc/cortx/s3/conf/s3config.yaml

# Increase retry interval
sed -i \
  -e 's/S3_SERVER_BGDELETE_BIND_ADDR *:.*/S3_SERVER_BGDELETE_BIND_ADDR: 0.0.0.0/' \
  -e 's/S3_MOTR_RECONNECT_RETRY_COUNT *:.*/S3_MOTR_RECONNECT_RETRY_COUNT: 30/' \
  /etc/cortx/s3/conf/s3config.yaml

# 'manual' copy of S3 FID files, until we integrate with hare - FIXME
cat s3server/s3server-1 > /etc/cortx/s3/s3server-1
cat s3server/s3server-2 > /etc/cortx/s3/s3server-2
cat s3server/s3server-3 > /etc/cortx/s3/s3server-3
cat s3server/s3server-4 > /etc/cortx/s3/s3server-4
cat s3server/s3server-5 > /etc/cortx/s3/s3server-5


# ##################
# # S3 BG Services #
# ##################
# 
# mkdir -p /var/log/cortx/s3/s3backgrounddelete \
#          /etc/cortx/s3/s3backgrounddelete/
# 
# cp "$s3_repo_dir/s3backgrounddelete/s3backgrounddelete/config/s3_background_delete_config.yaml.sample" \
#    /etc/cortx/s3/s3backgrounddelete/config.yaml
# cp "$s3_repo_dir/s3backgrounddelete/s3backgrounddelete/config/s3_cluster.yaml.sample" \
#    /etc/cortx/s3/s3backgrounddelete/s3_cluster.yaml

set_bg_config_param() {
  key="$1"
  val="$2"
  sed -i -e "s,$key:.*,$key: $val," /etc/cortx/s3/s3backgrounddelete/config.yaml
}

# 'manual' fix until s3_setup is able to handle BG Producer - FIXME
set_bg_config_param  producer_endpoint  '"http://192.168.134.219:28049"'

# adjust delay delete parameters to speed up testing
set_bg_config_param  scheduler_schedule_interval      60
set_bg_config_param  leak_processing_delay_in_mins     1
set_bg_config_param  version_processing_delay_in_mins  1
set_bg_config_param  consumer_sleep  1
set_bg_config_param  purge_sleep     1

# # create bgdelete account
# 
# access_key=`kube_run s3cipher generate_key --const_key s3backgroundaccesskey --key_len 22 --use_base64`
# secret_key=`kube_run s3cipher generate_key --const_key s3backgroundsecretkey --key_len 40`
# 
# cat ldif/bgdelete_account.ldif.template | \
#   sed -e "s,<access-key>,$access_key," \
#       -e "s,<secret-key>,$secret_key," \
#   > ldif/bgdelete_account.ldif
# 
# ldapadd -x -D "cn=admin,dc=seagate,dc=com" -w ldapadmin -f ldif/bgdelete_account.ldif -H "ldap://$OPENLDAP_SVC"
# 
# # create s3 topic 'bgdelete'
# kube_run python3 "$src_dir/message-bus/create-bgdelete-topic.py"


###########################################################################
# Destroy SHIM POD #
####################

kubectl delete -f k8s-blueprints/shim-pod.yaml

add_separator SUCCESSFULLY CREATED CONFIGS FOR S3 CONTAINERS.
