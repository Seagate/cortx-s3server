#!/bin/sh -x

# install openldap server and client
yum install -y openldap-servers openldap-clients

PASSWORD="seagate"
if [ "$1" != "--defaultpasswd" ]
then
    echo -n "Enter Password for LDAP: "
    read -s PASSWORD
fi

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

# add S3 schema
ldapadd -x -D "cn=admin,cn=config" -w $PASSWORD -f cn\=\{1\}s3user.ldif

# initialize ldap
ldapadd -x -D "cn=admin,dc=seagate,dc=com" -w $PASSWORD -f ldap-init.ldif
