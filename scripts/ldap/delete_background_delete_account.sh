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

USAGE="USAGE: delete_background_delete_account.sh <ldap passwd> "

if [ "$#" -ne 1 ]; then
    echo "$USAGE"
    exit 1
fi

ldap_passwd=$1
s3background_cofig="/opt/seagate/cortx/s3/s3backgrounddelete/config.yaml"

access_key=$(s3cipher --use_base64 --key_len  22  --const_key  "s3backgroundaccesskey")
if [[ $? -ne 0 ]]
then
    access_key=$(awk '/background_account_access_key/ {print}' $s3background_cofig  | cut -d " " -f 5 | sed -e 's/^"//' -e 's/"$//')
fi

ldapdelete -x -w $ldap_passwd -r "ak=$access_key,ou=accesskeys,dc=s3,dc=seagate,dc=com" -D "cn=sgiamadmin,dc=seagate,dc=com" -H ldapi:///

ldapdelete -x -w $ldap_passwd -r "o=s3-background-delete-svc,ou=accounts,dc=s3,dc=seagate,dc=com" -D "cn=sgiamadmin,dc=seagate,dc=com" -H ldapi:///

