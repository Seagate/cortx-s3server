#!/bin/sh
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

set -x

SCRIPT_PATH=$(readlink -f "$0")
SCRIPT_DIR=$(dirname "$SCRIPT_PATH")

motr_script_path=$SCRIPT_DIR/../third_party/motr/scripts
if [[ -d "$motr_script_path" && -f "$motr_script_path/install-build-deps" ]];
then
    sh $motr_script_path/install-build-deps
else
    # third_party submodules are not loaded yet hence do fresh clone and
    # run install-build-deps script.
    # This needs GitHub id and Personal Access Token
    git clone https://github.com/Seagate/cortx-motr.git
    ./cortx-motr/scripts/install-build-deps
    rm -rf cortx-motr
fi
