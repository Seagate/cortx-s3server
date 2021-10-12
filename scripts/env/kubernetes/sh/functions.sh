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

safe_grep() {
  # by default grep returns nonzero error code if no match found.
  # this functin will always return success from grep invocation.
  grep "$@" || true
}

###########################################################################
### k8s-specific functions ###
##############################

set_var_SVC_IP() {
  service_name="$1"
  SVC_IP="$(kubectl get svc "$service_name" | grep -v ^NAME | awk '{print $3}')"
  if ! validate_ip "$SVC_IP"; then
    add_separator "FAILED. service endpoint '$service_name' is not valid"
    kubectl get svc "$service_name"
    false
    return
  fi
}

set_var_SVC_EXTERNAL_IP() {
  service_name="$1"
  SVC_EXTERNAL_IP="$(kubectl get svc "$service_name" | grep -v ^NAME | awk '{print $4}')"
  if ! validate_ip "$SVC_EXTERNAL_IP"; then
    add_separator "FAILED. service endpoint '$service_name' is not valid"
    kubectl get svc "$service_name"
    false
    return
  fi
}

set_var_POD_IP() {
  pod_name="$1"
  POD_IP="$(kubectl describe pod "$pod_name" | grep '^IP:' | awk '{print $2}')"
  if ! validate_ip "$POD_IP"; then
    add_separator "FAILED. Cannot derive POD IP address of POD named '$pod_name'"
    kubectl get pod "$pod_name" -o wide
    false
    return
  fi
}

pull_images_for_pod() {
  cat "$1" | safe_grep 'image:' | awk '{print $2}' | sed "s,',,g" \
    | safe_grep -v "2.0.0-${S3_CORTX_ALL_CUSTOM_CI_NUMBER}-custom-ci" \
    | xargs --no-run-if-empty -n1 docker pull
  # grep -v is needed to prevent downloading of locally-built image
}

wait_till_pod_is_Running() {
  pod_name="$1"
  shift
  ( set +x
    add_separator "Waiting till pod '$pod_name' becomes Running"
    while [ $(kubectl get pod "$@" | safe_grep "$pod_name" | grep Running | wc -l) -lt 1 ]; do
      echo
      kubectl get pod "$@" | grep 'NAME\|'"$pod_name"
      echo
      echo "Pod named '$pod_name' is not yet in Running state, re-checking ..."
      echo '(hit CTRL-C if it is taking too many iterations)'
      date
      sleep 5
    done
  )
}

delete_pod_if_exists() {
  pod_name="$1"
  if kubectl get pod "$pod_name" &>/dev/null; then
    kubectl delete pod "$pod_name"
  fi
}

replace_tags() {
  ( set -xeuo pipefail
    dn=$(dirname "$1")
    fn=$(basename "$1" .template)
    blueprint="$dn/$fn"
    cat "$1" \
      | sed \
            -e "s,<base-etc-cortx>,$BASE_CONFIG_PATH,g" \
            -e "s,<stub-etc-cortx>,$STUB_CONFIG_PATH,g" \
            -e "s,<symas-image>,'$SYMAS_IMAGE'," \
            -e "s/<vm-hostname>/${HOST_FQDN}/g" \
            -e "s,<s3-cortx-all-image>,ghcr.io/seagate/cortx-all:${S3_CORTX_ALL_IMAGE_TAG}," \
            -e "s,<motr-cortx-all-image>,ghcr.io/seagate/cortx-all:${MOTR_CORTX_ALL_IMAGE_TAG}," \
      > "$blueprint"
    if [ -n "${DO_APPLY+x}" ]; then
      if [ -n "${THIS_IS_POD+x}" ]; then
        pull_images_for_pod "$blueprint"
        delete_pod_if_exists  "$2"
      fi
      kubectl apply -f "$dn/$fn"
      if [ -n "${THIS_IS_POD+x}" ]; then
        wait_till_pod_is_Running  "$2"
      fi
    fi
  )
}

replace_tags_and_apply() {
  ( DO_APPLY=1
    replace_tags "$1"
  )
}

replace_tags_and_create_pod() {
  ( DO_APPLY=1
    THIS_IS_POD=1
    replace_tags "$1" "$2"
   )
}
