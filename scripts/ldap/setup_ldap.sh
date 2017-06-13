#!/bin/bash -e

##################################
# Install and configure OpenLDAP #
##################################

USAGE="USAGE: bash $(basename "$0") [--defaultpasswd] [--help | -h]
Install and configure OpenLDAP.

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

# install openldap server and client
yum install -y openldap-servers openldap-clients

PASSWORD="seagate"
if [[ $defaultpasswd == false ]]
then
    echo -n "Enter Password for LDAP: "
    read -s PASSWORD && [[ -z $PASSWORD ]] && echo 'Password can not be null.' && exit 1
fi

echo $PASSWORD

# generate encrypted password
SHA=$(slappasswd -s $PASSWORD)
ESC_SHA=$(echo $SHA | sed 's/[/]/\\\//g')
EXPR='s/$PASSWD/'$ESC_SHA'/g'

CFG_FILE=$(mktemp XXXX.ldif)
cp -f cfg_ldap.ldif $CFG_FILE
sed -i $EXPR $CFG_FILE

chkconfig slapd on

# restart slapd
systemctl start slapd

# configure LDAP
ldapmodify -Y EXTERNAL -H ldapi:/// -w $PASSWORD -f $CFG_FILE
rm -f $CFG_FILE

# restart slapd
systemctl start slapd

# delete the schema from LDAP.
rm -f /etc/openldap/slapd.d/cn\=config/cn\=schema/cn\=\{1\}s3user.ldif

# add S3 schema
ldapadd -x -D "cn=admin,cn=config" -w $PASSWORD -f cn\=\{1\}s3user.ldif

# initialize ldap
ldapadd -x -D "cn=admin,dc=seagate,dc=com" -w $PASSWORD -f ldap-init.ldif
