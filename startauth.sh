#!/bin/sh -x
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

if [ $# -ne 1 ]
then
  echo "Invalid number of arguments passed to the script"
  echo "Usage: startauth.sh <base config path>"
  exit 1
fi

cd $(dirname $0)

base_config_path=$1
echo "Base config path is $base_config_path"
java -jar AuthServer-1.0-0.jar $1
