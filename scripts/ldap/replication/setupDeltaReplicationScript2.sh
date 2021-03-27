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
##################################
# Configure ldap-replication
##################################

INSTALLDIR="/opt/seagate/cortx/s3/install/ldap/replication"
mkdir /var/lib/ldap/accesslog
chmod 777 /var/lib/ldap/accesslog

ldapadd -Y EXTERNAL -H ldapi:/// -f $INSTALLDIR/accesslog_config_delta.ldif
