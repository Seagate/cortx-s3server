#!/bin/bash
#
# Copyright (c) 2020 Seagate Technology LLC and/or its Affiliates
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

source ./config.sh
source ./env.sh
source ./sh/functions.sh

set -e # exit immediatly on errors
set -x # print each statement before execution

add_separator CONFIGURING HAPROXY CONTAINER.

kube_run() {
  kubectl exec -i depl-pod -c authserver -- "$@"
}

set_var_OPENLDAP_SVC

# Generate encrypted ldap password for sgiam-admin on Pod

const_key=`kube_run s3cipher generate_key --const_key cortx`
encrypted_pwd=`kube_run  s3cipher encrypt --data "ldapadmin" --key "$const_key"`

# Copy encrypted ldap-passsword (ldapLoginPW) and openldap endpoint (key name
# ldapHost) to authserver.properties file.

kube_run sed -i "s|^ldapLoginPW *=.*|ldapLoginPW=$encrypted_pwd|;
                 s|^ldapHost *=.*|ldapHost=$OPENLDAP_SVC|" \
             /opt/seagate/cortx/auth/resources/authserver.properties

kube_run mkdir -p /etc/cortx/s3/ \
                  /etc/cortx/s3/stx/ \
                  /etc/haproxy/errors

kube_run sh -c '/opt/seagate/cortx/auth/startauth.sh /opt/seagate/cortx &'

sleep 1

set +x
if [ -z "`kube_run ps ax | grep 'java -jar AuthServer'`" ]; then
  echo
  kube_run ps ax
  echo
  add_separator FAILED.  AuthServer does not seem to be running.
  false
fi
set -x

add_separator SUCCESSFULLY CONFIGURED AUTHSERVER CONTAINER.
