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



add_separator Validating BG consumer.

kube_run() {
  kubectl exec -i io-pod -c s3-bg-consumer -- "$@"
}

set +x
if [ "`kube_run ps ax | grep '/opt/seagate/cortx/s3/s3backgrounddelete/s3backgroundconsumer' | wc -l`" -ne 1 ]; then
  echo
  kubectl logs io-pod s3-bg-consumer
  echo
  kube_run ps ax
  echo
  add_separator FAILED.  S3 BG Consumer does not seem to be running properly.
  false
fi
set -x

add_separator SUCCESS - BG consumer up and running.



add_separator Validating BG producer.

kube_run() {
  kubectl exec -i s3-bg-producer-pod -c s3-bg-producer -- "$@"
}

set +x
if [ "`kube_run ps ax | grep '/opt/seagate/cortx/s3/s3backgrounddelete/s3backgroundproducer' | wc -l`" -ne 1 ]; then
  echo
  kubectl logs s3-bg-producer-pod s3-bg-producer
  echo
  kube_run ps ax
  echo
  add_separator FAILED.  S3 BG Producer does not seem to be running properly.
  false
fi
set -x

add_separator SUCCESS - BG producer up and running.

add_separator SUCCESSFULLY VALIDATED BG CONTAINERS.
