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
# configure OpenLDAP #
##################################


USAGE="USAGE: bash $(basename "$0") [--hostname] [--ldapadminpasswd <passwd>] [--rootdnpasswd <passwd>]
      [--help | -h]
Install and configure OpenLDAP.

where:
--hostname          host to configure
--ldapadminpasswd   optional ldapadmin password
--rootdnpasswd      optional rootdn password
--help              display this help and exit
"

set -e

LDAPADMINPASS=
ROOTDNPASSWORD=
host=

echo "Running s3_setup_ldap.sh script"
if [ $# -lt 1 ]
then
  echo "$USAGE"
  exit 1
fi

while test $# -gt 0
do
  case "$1" in
    --hostname ) shift;
        host=$1
        ;;
    --ldapadminpasswd ) shift;
        LDAPADMINPASS=$1
        ;;
    --rootdnpasswd ) shift;
        ROOTDNPASSWORD=$1
        ;;
    --help | -h )
        echo "$USAGE"
        exit 1
        ;;
  esac
  shift
done

if [ -z "$host" ]
then
    echo "Hostname can not be null."
    exit 1
fi

if [ -z "$LDAPADMINPASS" ]
then
    echo "Password can not be null."
    exit 1
fi

if [ -z "$ROOTDNPASSWORD" ]
then
    echo "Password can not be null."
    exit 1
fi

op=$(ldapsearch -w "$ROOTDNPASSWORD" -x -D cn=admin,cn=config -b cn=schema,cn=config -h "$host")

if [[ $op == *"s3user"* ]];then
    echo "Skipping s3 schema configuration as its already present on ${host}"
    exit 0
fi

INSTALLDIR="/opt/seagate/cortx/s3/install/ldap"
# generate encrypted password for ldap admin
#SHA=$(slappasswd -s "$LDAPADMINPASS")
#ESC_SHA=$(echo "$SHA" | sed 's/[/]/\\\//g')
ESC_SHA=$LDAPADMINPASS
EXPR='s/userPassword: *.*/userPassword: '$ESC_SHA'/g'
ADMIN_USERS_FILE=$(mktemp XXXX.ldif)
cp -f "$INSTALLDIR"/iam-admin.ldif "$ADMIN_USERS_FILE"
sed -i "$EXPR" "$ADMIN_USERS_FILE"

# Commenting this since chkconfig uses systemd utility which is not available in kubernetes env.
#chkconfig slapd on

# add S3 schema
ldapadd -x -D "cn=admin,cn=config" -w "$ROOTDNPASSWORD" -f "$INSTALLDIR"/cn\=s3user.ldif -h "$host"


# initialize ldap
ldapadd -x -D "cn=admin,dc=seagate,dc=com" -w "$ROOTDNPASSWORD" -f "$INSTALLDIR"/s3-ldap-init.ldif -h "$host" || /bin/true

# Setup iam admin and necessary permissions
ldapadd -x -D "cn=admin,dc=seagate,dc=com" -w "$ROOTDNPASSWORD" -f "$ADMIN_USERS_FILE" -h "$host" || /bin/true
rm -f $ADMIN_USERS_FILE

ldapmodify -x -D "cn=admin,cn=config" -w "$ROOTDNPASSWORD" -f "$INSTALLDIR"/s3-iam-admin-access.ldif -h "$host"

# Enable slapd log with logLevel as "none"
# for more info : http://www.openldap.org/doc/admin24/slapdconfig.html
echo "Enable slapd log with logLevel"
ldapmodify -x -D "cn=admin,cn=config" -w "$ROOTDNPASSWORD" -f "$INSTALLDIR"/slapdlog.ldif -h "$host"
# Apply indexing on keys for performance improvement
ldapmodify -x -D "cn=admin,cn=config" -w "$ROOTDNPASSWORD" -f "$INSTALLDIR"/s3slapdindex.ldif -h "$host"

# Set ldap search Result size
ldapmodify -x -D "cn=admin,cn=config" -w "$ROOTDNPASSWORD" -f "$INSTALLDIR"/resultssizelimit.ldif -h "$host"

echo "************************************************************"
echo "You may have to redo any selinux settings as selinux-policy package was updated."
echo "Example for nginx: setsebool httpd_can_network_connect on -P"
echo "************************************************************"


