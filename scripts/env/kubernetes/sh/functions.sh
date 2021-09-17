# not for execution -- only for using with `source` command.

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

###########################################################################
### Generic functions ###
#########################

add_separator() {
  ( set +x +e
    echo -e '\n\n\n========================= '"$*"' =========================\n'
  )
}

self_check() {
  ( set +x +e
    add_separator Self-check
    while true; do
      read -p "$* (yes/no) [no] " var
      if test x"$var" = xy -o x"$var" = xyes; then
        true
        return
      fi
      if test x"$var" = x -o x"$var" = xn -o x"$var" = xno; then
        false
        return
      fi
    done
    false
    return
  )
}


hit_enter_when_done() {
  ( set +x +e
    add_separator
    echo "$*"
    read -p 'Hit Enter when done' tmp
  )
}

validate_ip() {
  if [[ ! "$1" =~ ^[0-9]{1,3}(\.[0-9]{1,3}){3}$ ]]; then
    echo "Not a valid IP address: <$1>"
    false
  fi
  true
}

###########################################################################
### k8s-specific functions ###
##############################

set_var_OPENLDAP_SVC() {
  OPENLDAP_SVC=`kubectl get svc openldap-svc | grep ldap | awk '{print $3}'`
  if ! validate_ip "$OPENLDAP_SVC"; then
    add_separator "FAILED. openldap service endpoint is not accessible"
    kubectl get svc openldap-svc
    false
  fi
}

set_var_DEPL_POD_IP() {
  DEPL_POD_IP=`kubectl get pod depl-pod -o wide | grep -v ^NAME | awk '{print $6}'`
  if ! validate_ip "$DEPL_POD_IP"; then
    add_separator "FAILED. Cannot derive IO POD IP address"
    kubectl get pod depl-pod -o wide
    false
  fi
}
