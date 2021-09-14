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

set -e
set -x

add_separator "Pulling image at tag <$CORTX_ALL_IMAGE_TAG>"
docker pull "ghcr.io/seagate/cortx-all:$CORTX_ALL_IMAGE_TAG"
set +x
if [ -z "`docker images | grep 'cortx-all'`" ]; then
  echo
  docker images
  echo
  echo 'cortx-all image did not get downloaded (not in the list)'
  false
fi

add_separator SUCCESSFULLY PULLED CORTX-ALL IMAGE
