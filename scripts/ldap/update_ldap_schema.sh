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

# Script to update schema and initialize ldap for S3 Authserver.
# CAUTION: This scipt will delete existing S3 user data.

USAGE="USAGE: bash $(basename "$0") [--defaultpasswd] [-h | --help]
Update S3 schema in OpenLDAP.

where:
--defaultpasswd     use default password i.e. 'seagate' for LDAP
--help              display this help and exit"

defaultpasswd=false
case "$1" in
    --defaultpasswd )
        defaultpasswd=true
        ;;
    --help | -h )
        echo "$USAGE"
        exit 1
        ;;
esac

PASSWORD="seagate"
if [[ $defaultpasswd == false ]]
then
    echo -n "Enter Password for LDAP: "
    read -s PASSWORD && [[ -z $PASSWORD ]] && echo 'Password can not be null.' && exit 1
fi

# Delete all the entries from LDAP.
ldapdelete -x -w $PASSWORD -D "cn=admin,dc=seagate,dc=com" -r "dc=seagate,dc=com"

# Stop slapd
systemctl stop slapd

# Delete the schema from LDAP.
rm -f /etc/openldap/slapd.d/cn\=config/cn\=schema/cn\=\{1\}s3user.ldif

# Start slapd
systemctl start slapd

# Add S3 schema
ldapadd -x -w $PASSWORD -D "cn=admin,cn=config" -f cn\=\{1\}s3user.ldif

# Restart slapd to update the changes
systemctl restart slapd

# Initialize ldap
ldapadd -x -w $PASSWORD -D "cn=admin,dc=seagate,dc=com" -f ldap-init.ldif
