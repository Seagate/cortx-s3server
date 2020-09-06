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


set -e

# This script can be used to update encrpted password in authserver.properties
# file. Performs below steps
# 1. Encrypts given password using /opt/seagate/cortx/auth/AuthPassEncryptCLI-1.0-0.jar
# 2. Updates encrypted password in given authserver.properites file
# 3. Updates ldap password if provided by user

USAGE="USAGE: bash $(basename "$0") -l <ldap passwd>
          -p <authserver.properties file path>
          -c <new ldap password to be set>  [-h]

Encrypt ldap passwd and update in given authserver.properties file and change ldappasswd
provide either of -l and -c according to requirement, -p is mandatory.
in case of change of ldap password, restart slapd after this script.
"

if [ "$#" -ne 4 ]; then
    echo "$USAGE"
    exit 1
fi

ldap_passwd=
auth_properties=
encrypt_cli=/opt/seagate/cortx/auth/AuthPassEncryptCLI-1.0-0.jar
change_ldap_passwd=false

if rpm -q "salt"  > /dev/null 2>&1;
then
    # Release/Prod environment
    ldap_root_pwd=$(salt-call pillar.get openldap:admin:secret --output=newline_values_only)
    ldap_root_pwd=$(salt-call lyveutil.decrypt openldap "${ldap_root_pwd}" --output=newline_values_only)
else
    # Dev environment. Read ldap admin password from "/root/.s3_ldap_cred_cache.conf"
    source /root/.s3_ldap_cred_cache.conf
fi

# read the options
while getopts ":l:p:c:h:" o; do
    case "${o}" in
        l)
            ldap_passwd=${OPTARG}
            ;;
        p)
            auth_properties=${OPTARG}
            ;;
        c)
            change_ldap_passwd=true
            ldap_passwd=${OPTARG}
            ;;
        *)
            echo "$USAGE"
            exit 0
            ;;
    esac
done
shift $((OPTIND-1))

if [ ! -e $encrypt_cli ]; then
    echo "$encrypt_cli does not exists."
    exit 1
fi

if [ ! -e $auth_properties ]; then
    echo "$auth_properties file does not exists."
    exit 1
fi

if [ -z $ldap_passwd ]; then
    echo "Ldap password not specified."
    echo "$USAGE"
    exit 1
fi


# generate encrypted password for ldap admin
if [ "$change_ldap_passwd" = true ] ; then
    SHA=$(slappasswd -s $ldap_passwd)
    ESC_SHA=$(echo $SHA | sed 's/[/]/\\\//g')
    EXPR='s/{{ ldapadminpasswdhash.stdout }}/'$ESC_SHA'/g'
    ADMIN_USERS_FILE=$(mktemp XXXX.ldif)
    cp -f change_ldap_passwd.ldif $ADMIN_USERS_FILE
    sed -i "$EXPR" $ADMIN_USERS_FILE
    # Setup iam admin and necessary permissions
    ldapadd -x -D "cn=admin,dc=seagate,dc=com" -w "$ldap_root_pwd" -f "$ADMIN_USERS_FILE"
    rm -f $ADMIN_USERS_FILE
    echo -e "\n OpenLdap password Updated Successfully,You need to Restart Slapd"
fi

# Encrypt the password
encrypted_pass=`java -jar $encrypt_cli -s $ldap_passwd`

# Update the config
escaped_pass=`echo "$encrypted_pass" | sed 's/\//\\\\\\//g'`
sed -i "s/^ldapLoginPW=.*/ldapLoginPW=${escaped_pass}/" $auth_properties
