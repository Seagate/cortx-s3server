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
# 1. Encrypts given password using s3cipher commands
# 2. Updates encrypted password in given authserver.properites file
# 3. Updates ldap password if provided by user

USAGE="USAGE: bash $(basename "$0") -t <host to configure>
          -l <ldap passwd>
          -p <authserver.properties file path>
          -c <new ldap password to be set>  [-h]

Encrypt ldap passwd and update in given authserver.properties file and change ldappasswd
provide either of -l and -c according to requirement, -p is mandatory.
in case of change of ldap password, restart slapd after this script.
"

ldap_passwd=
auth_properties=
change_ldap_passwd=false
hostname=

# read the options
while getopts ":t:l:p:c:h:" o; do
    case "${o}" in
        t)
            hostname=${OPTARG}
            ;;
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

if [ ! command -v s3cipher &>/dev/null ]; then
    echo "s3cipher utility does not exists."
    exit 1
fi

if [ -z $auth_properties ]; then
    echo "AuthServer properties file not specified."
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
#if host not specified then use localhost
if [ -z $hostname ]; then
    hostname=$HOSTNAME
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
    ldapadd -x -D "cn=admin,dc=seagate,dc=com" -w "$ldap_passwd" -f "$ADMIN_USERS_FILE" -h "$hostname"
    rm -f $ADMIN_USERS_FILE
    echo -e "\n OpenLdap password Updated Successfully,You need to Restart Slapd"
fi

# Encrypt the password using s3cipher
ldapcipherkey=$(s3cipher generate_key --const_key="cortx")
encrypted_pass=$(s3cipher encrypt --data="$ldap_passwd" --key="$ldapcipherkey")

# Update the config
escaped_pass=`echo "$encrypted_pass" | sed 's/\//\\\\\\//g'`
s3confstore "properties://$auth_properties" setkey --key ldapLoginPW --value ${escaped_pass}
