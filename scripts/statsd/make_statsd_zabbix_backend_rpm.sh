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


# This scripts creates RPM from statsd-zabbix-backend source
# using the SPEC file.
set -e

CURRENT_DIR=`pwd`
rm -rf ~/rpmbuild
rpmdev-setuptree
wget https://github.com/parkerd/statsd-zabbix-backend/archive/v0.2.0.tar.gz
mv v0.2.0.tar.gz ~/rpmbuild/SOURCES/
cp statsd_zabbix_backend.spec ~/rpmbuild/SPECS/
rpmbuild -ba ~/rpmbuild/SPECS/statsd_zabbix_backend.spec
cd "$CURRENT_DIR"
echo "RPM created successfully in ~/rpmbuild/RPMS dir"
