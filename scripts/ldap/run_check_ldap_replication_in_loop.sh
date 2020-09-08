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
if rpm -q "salt" > /dev/null 2>&1;
    # Release/prod envirionment
    ldap_admin_pwd=$(salt-call pillar.get openldap:iam_admin:secret --output=newline_values_only)
    ldap_admin_pwd=$(salt-call lyveutil.decrypt openldap "${ldap_admin_pwd}" --output=newline_values_only)
then
    # Dev environment. Read ldap admin password from "/root/.s3_ldap_cred_cache.conf"
    source /root/.s3_ldap_cred_cache.conf
fi

a=0
while true
do
        ./check_ldap_replication.sh -s hostname.txt -p "$ldap_admin_pwd"
        a=$(( $a + 1 ))
        if [ $a -eq 500 ]
        then
                break
        fi
done
