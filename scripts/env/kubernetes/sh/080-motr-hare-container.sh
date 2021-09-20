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

add_separator CONFIGURING MOTR-HARE CONTAINER.

# kube_run() {
#   kubectl exec -i depl-pod -c hare-motr -- "$@"
# }
# 
# set +x
# if [ "`kube_run ps ax | grep /usr/bin/m0d | wc -l`" -ne 3 ]; then
#   echo
#   kube_run ps ax
#   echo
#   add_separator FAILED.  Motr does not seem to be running properly.
#   false
# fi
# set -x

add_separator SUCCESSFULLY CONFIGURED MOTR-HARE CONTAINER.
