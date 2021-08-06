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

USAGE="USAGE: bash $(basename "$0") [--ldapadminpasswd <passwd>] [--rootdnpasswd <passwd>] [--defaultpasswd] [--skipssl]
      [--forceclean] [--help | -h]
Install and configure OpenLDAP.

where:
--ldapadminpasswd   optional ldapadmin password
--rootdnpasswd      optional rootdn password
--defaultpasswd     optional set default password
--skipssl           skips all ssl configuration for LDAP
--forceclean        Clean old openldap setup (** careful: deletes data **)
--help              display this help and exit
NOTE: If either one or both --ldapadminpasswd and --rootdnpasswd are not provided and --defaultpasswd is not provided, runtime input will be required from the user."

set -e
defaultpasswd=false
usessl=true
forceclean=false
LDAPADMINPASS=
ROOTDNPASSWORD=
defaultpasswds="ldapadmin"

echo "Running setup_ldap.sh script"
if [ $# -lt 1 ]
then
  echo "$USAGE"
  exit 1
fi

while test $# -gt 0
do
  case "$1" in
    --ldapadminpasswd ) shift;
        LDAPADMINPASS=$1
        ;;
    --rootdnpasswd ) shift;
        ROOTDNPASSWORD=$1
        ;;
    --defaultpasswd )
        defaultpasswd=true
        ;;
    --skipssl )
        usessl=false
        ;;
    --forceclean )
        forceclean=true
        ;;
    --help | -h )
        echo "$USAGE"
        exit 1
        ;;
  esac
  shift
done

INSTALLDIR="/opt/seagate/cortx/s3/install/ldap"
# Clean up old configuration if any for idempotency
# Removing schemas
rm -f /etc/openldap/slapd.d/cn\=config/cn\=schema/cn\=\{2\}s3user.ldif
rm -f /etc/openldap/slapd.d/cn\=config/cn\=schema/*ppolicy.ldif
# Removing all loaded modules in ldap
rm -rf /etc/openldap/slapd.d/cn\=config/cn\=module\{0\}.ldif
rm -rf /etc/openldap/slapd.d/cn\=config/cn\=module\{1\}.ldif
rm -rf /etc/openldap/slapd.d/cn\=config/cn\=module\{2\}.ldif
# Removing mdb configuration
rm -rf /etc/openldap/slapd.d/cn\=config/olcDatabase\=\{2\}mdb
rm -rf /etc/openldap/slapd.d/cn\=config/olcDatabase\=\{2\}mdb.ldif

# Data Cleanup
if [[ $forceclean == true ]]
then
  rm -rf /var/lib/ldap/*
fi

#removes all hdb file instances if they exist, non-existence of files doesn't throw an error as well.
rm -f /etc/openldap/slapd.d/cn\=config/olcDatabase=*hdb.ldif
cp -f $INSTALLDIR/olcDatabase\=\{2\}mdb.ldif /etc/openldap/slapd.d/cn\=config/

chgrp ldap /etc/openldap/certs/password # onlyif: grep -q ldap /etc/group && test -f /etc/openldap/certs/password

if [ -z "$LDAPADMINPASS" ]
then
    # If --defaultpasswd is set, use it. Else ask from user as input
    if [[ $defaultpasswd == true ]]
    then
        LDAPADMINPASS=$defaultpasswds
    else
        # Fetch password from User
        echo -en "\nEnter Password for LDAP IAM admin: "
        read -s LDAPADMINPASS && [[ -z $LDAPADMINPASS ]] && echo 'Password can not be null.' && exit 1
    fi
fi

if [ -z "$ROOTDNPASSWORD" ]
then
    # If --defaultpasswd is set, use it. Else ask from user as input
    if [[ $defaultpasswd == true ]]
    then
        ROOTDNPASSWORD=$defaultpasswds
    else
        # Fetch password from User
        echo -en "\nEnter Password for LDAP rootDN: "
        read -s ROOTDNPASSWORD && [[ -z $ROOTDNPASSWORD ]] && echo 'Password can not be null.' && exit 1
    fi
fi

# generate encrypted password for rootDN
SHA=$(slappasswd -s $ROOTDNPASSWORD)
ESC_SHA=$(echo $SHA | sed 's/[/]/\\\//g')
EXPR='s/olcRootPW: *.*/olcRootPW: '$ESC_SHA'/g'

CFG_FILE=$(mktemp XXXX.ldif)
cp -f $INSTALLDIR/cfg_ldap.ldif $CFG_FILE
sed -i "$EXPR" $CFG_FILE

# generate encrypted password for ldap admin
SHA=$(slappasswd -s $LDAPADMINPASS)
ESC_SHA=$(echo $SHA | sed 's/[/]/\\\//g')
EXPR='s/userPassword: *.*/userPassword: '$ESC_SHA'/g'
ADMIN_USERS_FILE=$(mktemp XXXX.ldif)
cp -f $INSTALLDIR/iam-admin.ldif $ADMIN_USERS_FILE
sed -i "$EXPR" $ADMIN_USERS_FILE

chkconfig slapd on

# start slapd
systemctl stop slapd
systemctl enable slapd
systemctl start slapd
echo "started slapd"

# configure LDAP
ldapmodify -Y EXTERNAL -H ldapi:/// -w $ROOTDNPASSWORD -f $CFG_FILE
rm -f $CFG_FILE


# initialize ldap
ldapadd -x -D "cn=admin,dc=seagate,dc=com" -w "$ROOTDNPASSWORD" -f "$INSTALLDIR"/ldap-init.ldif -H ldapi:/// || /bin/true

# Setup iam admin and necessary permissions
ldapadd -x -D "cn=admin,dc=seagate,dc=com" -w "$ROOTDNPASSWORD" -f "$ADMIN_USERS_FILE" -H ldapi:/// || /bin/true
rm -f $ADMIN_USERS_FILE

ldapmodify -Y EXTERNAL -H ldapi:/// -w $ROOTDNPASSWORD -f $INSTALLDIR/iam-admin-access.ldif

# Enable IAM constraints
ldapadd -Y EXTERNAL -H ldapi:/// -w $ROOTDNPASSWORD -f $INSTALLDIR/iam-constraints.ldif

#Enable ppolicy schema
ldapmodify -D "cn=admin,cn=config" -w $ROOTDNPASSWORD -a -f /etc/openldap/schema/ppolicy.ldif -H ldapi:///

# Enable password policy and configure
ldapmodify -D "cn=admin,cn=config" -w $ROOTDNPASSWORD -a -f $INSTALLDIR/ppolicymodule.ldif -H ldapi:///

ldapmodify -D "cn=admin,cn=config" -w $ROOTDNPASSWORD -a -f $INSTALLDIR/ppolicyoverlay.ldif -H ldapi:///

ldapmodify -x -a -H ldapi:/// -D cn=admin,dc=seagate,dc=com -w "$ROOTDNPASSWORD" -f "$INSTALLDIR"/ppolicy-default.ldif || /bin/true

# add S3 schema
ldapadd -x -D "cn=admin,cn=config" -w $ROOTDNPASSWORD -f $INSTALLDIR/cn\=\{2\}s3user.ldif -H ldapi:///


# Enable slapd log with logLevel as "none"
# for more info : http://www.openldap.org/doc/admin24/slapdconfig.html
echo "Enable slapd log with logLevel"
ldapmodify -Y EXTERNAL -H ldapi:/// -w $ROOTDNPASSWORD -f $INSTALLDIR/slapdlog.ldif
# Apply indexing on keys for performance improvement
ldapmodify -Y EXTERNAL -H ldapi:/// -w $ROOTDNPASSWORD -f $INSTALLDIR/s3slapdindex.ldif

# Set ldap search Result size
ldapmodify -Y EXTERNAL -H ldapi:/// -w $ROOTDNPASSWORD -f $INSTALLDIR/resultssizelimit.ldif

echo "Encrypting Authserver LDAP password.."
/opt/seagate/cortx/auth/scripts/enc_ldap_passwd_in_cfg.sh -l $LDAPADMINPASS -p /opt/seagate/cortx/auth/resources/authserver.properties

echo "Restart S3authserver.."
systemctl restart s3authserver

if [[ $usessl == true ]]
then
#Deploy SSL certificates and enable OpenLDAP SSL port
./ssl/enable_ssl_openldap.sh -cafile /etc/ssl/stx-s3/openldap/ca.crt \
                   -certfile /etc/ssl/stx-s3/openldap/s3openldap.crt \
                   -keyfile /etc/ssl/stx-s3/openldap/s3openldap.key
fi

echo "************************************************************"
echo "You may have to redo any selinux settings as selinux-policy package was updated."
echo "Example for nginx: setsebool httpd_can_network_connect on -P"
echo "************************************************************"
