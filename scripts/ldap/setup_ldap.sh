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
yum list installed selinux-policy && yum update -y selinux-policy
yum install -y openldap-servers openldap-clients

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
ldapadd -x -D "cn=admin,cn=config" -w $ROOTDNPASSWORD -f cn\=\{1\}s3user.ldif

# initialize ldap
ldapadd -x -D "cn=admin,dc=seagate,dc=com" -w $ROOTDNPASSWORD -f ldap-init.ldif

# Setup iam admin and necessary permissions
ldapadd -x -D "cn=admin,dc=seagate,dc=com" -w $ROOTDNPASSWORD -f $ADMIN_USERS_FILE
rm -f $ADMIN_USERS_FILE

ldapmodify -Y EXTERNAL -H ldapi:/// -w $ROOTDNPASSWORD -f iam-admin-access.ldif

echo "************************************************************"
echo "You may have to redo any selinux settings as selinux-policy package was updated."
echo "Example for nginx: setsebool httpd_can_network_connect on -P"
echo "************************************************************"
