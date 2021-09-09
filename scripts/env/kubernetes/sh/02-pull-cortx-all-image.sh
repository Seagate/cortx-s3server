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
source ./sh/functions.sh

set -e

add_separator "Pulling image at tag <$CORTX_ALL_IMAGE_TAG>"

set -x
docker pull "ghcr.io/seagate/cortx-all:$CORTX_ALL_IMAGE_TAG"

set +x

add_separator "Listing CORTX images"

docker images | grep 'REPOSITORY\|cortx-all'

self_check "Is cortx-all image listed above?"

add_separator SUCCESSFULLY PULLED CORTX-ALL IMAGE
