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
# Install and configure OpenLDAP #
##################################

USAGE="USAGE: bash $(basename "$0") [--defaultpasswd] [--skipssl]
      [--forceclean] [--help | -h]
Install and configure OpenLDAP.

where:
--defaultpasswd     use default password i.e. 'seagate' for LDAP
--skipssl           skips all ssl configuration for LDAP
--forceclean        Clean old openldap setup (** careful: deletes data **)
--help              display this help and exit"

set -e
defaultpasswd=false
usessl=true
forceclean=false

if [ $# -lt 1 ]
then
  echo "$USAGE"
  exit 1
fi

while test $# -gt 0
do
  case "$1" in
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


# install openldap server and client
yum list installed selinux-policy && yum update -y selinux-policy

# Clean up old setup if any
if [[ $forceclean == true ]]
then
  systemctl stop slapd 2>/dev/null || /bin/true
  yum remove -y openldap-servers openldap-clients || /bin/true
  rm -f /etc/openldap/slapd.d/cn\=config/cn\=schema/cn\=\{1\}s3user.ldif
  rm -rf /var/lib/ldap/*
  rm -f /etc/sysconfig/slapd* || /bin/true
  rm -f /etc/openldap/slapd* || /bin/true
  rm -rf /etc/openldap/slapd.d/*
fi

yum install -y openldap-servers openldap-clients
cp -f olcDatabase\=\{2\}mdb.ldif /etc/openldap/slapd.d/cn\=config/

ROOTDNPASSWORD="seagate"
LDAPADMINPASS="ldapadmin"
if [[ $defaultpasswd == false ]]
then
    echo -en "\nEnter Password for LDAP rootDN: "
    read -s ROOTDNPASSWORD && [[ -z $ROOTDNPASSWORD ]] && echo 'Password can not be null.' && exit 1

    echo -en "\nEnter Password for LDAP IAM admin: "
    read -s LDAPADMINPASS && [[ -z $LDAPADMINPASS ]] && echo 'Password can not be null.' && exit 1
fi

# generate encrypted password for rootDN
SHA=$(slappasswd -s $ROOTDNPASSWORD)
ESC_SHA=$(echo $SHA | sed 's/[/]/\\\//g')
EXPR='s/{{ slapdpasswdhash.stdout }}/'$ESC_SHA'/g'

CFG_FILE=$(mktemp XXXX.ldif)
cp -f cfg_ldap.ldif $CFG_FILE
sed -i "$EXPR" $CFG_FILE

# generate encrypted password for ldap admin
SHA=$(slappasswd -s $LDAPADMINPASS)
ESC_SHA=$(echo $SHA | sed 's/[/]/\\\//g')
EXPR='s/{{ ldapadminpasswdhash.stdout }}/'$ESC_SHA'/g'

ADMIN_USERS_FILE=$(mktemp XXXX.ldif)
cp -f iam-admin.ldif $ADMIN_USERS_FILE
sed -i "$EXPR" $ADMIN_USERS_FILE

chkconfig slapd on

# restart slapd
systemctl start slapd

# configure LDAP
ldapmodify -Y EXTERNAL -H ldapi:/// -w $ROOTDNPASSWORD -f $CFG_FILE
rm -f $CFG_FILE

# restart slapd
systemctl start slapd

# delete the schema from LDAP.
rm -f /etc/openldap/slapd.d/cn\=config/cn\=schema/cn\=\{1\}s3user.ldif

# add S3 schema
ldapadd -x -D "cn=admin,cn=config" -w $ROOTDNPASSWORD -f cn\=\{1\}s3user.ldif -H ldapi:///

# initialize ldap
ldapadd -x -D "cn=admin,dc=seagate,dc=com" -w $ROOTDNPASSWORD -f ldap-init.ldif -H ldapi:///

# Setup iam admin and necessary permissions
ldapadd -x -D "cn=admin,dc=seagate,dc=com" -w $ROOTDNPASSWORD -f $ADMIN_USERS_FILE -H ldapi:///
rm -f $ADMIN_USERS_FILE

ldapmodify -Y EXTERNAL -H ldapi:/// -w $ROOTDNPASSWORD -f iam-admin-access.ldif

# Enable IAM constraints
ldapadd -Y EXTERNAL -H ldapi:/// -w $ROOTDNPASSWORD -f iam-constraints.ldif

#Enable ppolicy schema
ldapmodify -D "cn=admin,cn=config" -w $ROOTDNPASSWORD -a -f /etc/openldap/schema/ppolicy.ldif -H ldapi:///

# Enable password policy and configure
ldapmodify -D "cn=admin,cn=config" -w $ROOTDNPASSWORD -a -f ppolicymodule.ldif -H ldapi:///

ldapmodify -D "cn=admin,cn=config" -w $ROOTDNPASSWORD -a -f ppolicyoverlay.ldif -H ldapi:///

ldapmodify -x -a -H ldapi:/// -D cn=admin,dc=seagate,dc=com -w $ROOTDNPASSWORD -f ppolicy-default.ldif

# Enable slapd log with logLevel as "none"
# for more info : http://www.openldap.org/doc/admin24/slapdconfig.html
ldapmodify -Y EXTERNAL -H ldapi:/// -w $ROOTDNPASSWORD -f slapdlog.ldif
# Apply indexing on keys for performance improvement
ldapmodify -Y EXTERNAL -H ldapi:/// -w $ROOTDNPASSWORD -f s3slapdindex.ldif

# Rstart slapd
systemctl restart slapd

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
