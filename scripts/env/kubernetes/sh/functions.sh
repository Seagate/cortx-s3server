# not for execution -- only for using with `source` command.

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
    return
  fi
  true
}

###########################################################################
### k8s-specific functions ###
##############################

set_var_SVC_IP() {
  service_name="$1"
  SVC_IP=`kubectl get svc "$service_name" | grep -v ^NAME | awk '{print $3}'`
  if ! validate_ip "$SVC_IP"; then
    add_separator "FAILED. service endpoint '$service_name' is not valid"
    kubectl get svc "$service_name"
    false
    return
  fi
}

set_var_SVC_EXTERNAL_IP() {
  service_name="$1"
  SVC_EXTERNAL_IP=`kubectl get svc "$service_name" | grep -v ^NAME | awk '{print $4}'`
  if ! validate_ip "$SVC_EXTERNAL_IP"; then
    add_separator "FAILED. service endpoint '$service_name' is not valid"
    kubectl get svc "$service_name"
    false
    return
  fi
}

set_var_POD_IP() {
  pod_name="$1"
  POD_IP=`kubectl describe pod "$pod_name" | grep '^IP:' | awk '{print $2}'`
  if ! validate_ip "$POD_IP"; then
    add_separator "FAILED. Cannot derive POD IP address of POD named '$pod_name'"
    kubectl get pod "$pod_name" -o wide
    false
    return
  fi
}

pull_images_for_pod() {
  cat "$1" | grep 'image:' | awk '{print $2}' | xargs -n1 docker pull
}
