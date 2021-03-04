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

json_prereqs_file="/opt/seagate/cortx/s3/mini-prov/s3setup_prereqs.json"

install_rpms () {
	command -v yum &> /dev/null || die_with_error "yum could not be found!"
	rpms=("$@")
	for rpm in "${rpms[@]}" ; do
		echo "Trying to install rpm: $rpm"
		yum install "$rpm" -y --nogpgcheck || die_with_error "$rpm install failed!"
	done
}

restart_services () {
	command -v systemctl &> /dev/null || die_with_error "systemctl could not be found!"
	services=("$@")
	for service in "${services[@]}" ; do
		echo "Trying to restart service: $service"
		systemctl restart "$service" || die_with_error "$service failed to start!"
	done
}

update_haproxy_cfg () {
	cfg_file="/etc/haproxy/haproxy.cfg"
	if [ ! -e $cfg_file ];then
		die_with_error "$cfg_file does not exists!"
	fi
	sed -e '/ssl-default-bind-ciphers PROFILE=SYSTEM/ s/^#*/#/' -i $cfg_file
	sed -e '/ssl-default-server-ciphers PROFILE=SYSTEM/ s/^#*/#/' -i $cfg_file
}

command -v jq &> /dev/null || die_with_error "jq could not be found!"

rpms_list=$(jq '.rpms' $json_prereqs_file)
rpms_list=$(echo "$rpms_list" | tr -d '[]"\r\n')
IFS=', ' read -r -a rpms <<< "$rpms_list"
if [ ${#rpms[@]} -eq 0 ];then
	die_with_error "jq '.rpms' $json_prereqs_file failed"
fi
if [ "${rpms[0]}" == "null" ];then
	die_with_error "jq '.rpms' $json_prereqs_file failed"
fi

services_list=$(jq '.services' $json_prereqs_file)
services_list=$(echo "$services_list" | tr -d '[]"\r\n')
IFS=', ' read -r -a services <<< "$services_list"
if [ ${#services[@]} -eq 0 ];then
	die_with_error "jq '.services' $json_prereqs_file failed"
fi
if [ "${services[0]}" == "null" ];then
	die_with_error "jq '.services' $json_prereqs_file failed"
fi

install_rpms "${rpms[@]}"
update_haproxy_cfg
restart_services "${services[@]}"
