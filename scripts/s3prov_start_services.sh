#!/bin/bash -e
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

die_with_error () {
	echo "$1 Exitting.." >&2
	exit -1
}

command -v systemctl &> /dev/null || die_with_error "systemctl could not be found!"

services=("$@")
for service in "${services[@]}" ; do
	echo "Trying to restart service: $service"
	systemctl restart "$service" || die_with_error "$service failed to start!"
done
