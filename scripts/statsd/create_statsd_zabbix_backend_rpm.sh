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


# This scripts creates RPM from statsd-zabbix-backend source using FPM

set -e
RPM_SANDBOX_DIR=rpmssandbox
RPM_PKG_NAME=statsd-zabbix-backend
RPM_VERSION=0.2.0

wget https://github.com/parkerd/statsd-zabbix-backend/archive/v0.2.0.tar.gz
tar xvzf v0.2.0.tar.gz
mkdir -p $RPM_SANDBOX_DIR/usr/lib/node_modules/$RPM_PKG_NAME/
cp -r statsd-zabbix-backend-0.2.0/* $RPM_SANDBOX_DIR/usr/lib/node_modules/$RPM_PKG_NAME/

fpm -s dir -t rpm -C $RPM_SANDBOX_DIR \
    --name $RPM_PKG_NAME \
    --version $RPM_VERSION \
    --directories /usr/lib/node_modules/$RPM_PKG_NAME \
    --description "StatsD Zabbix Backend"
