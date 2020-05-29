#!/bin/sh

USAGE="USAGE: create_background_delete_account.sh <ldap passwd> "

if [ "$#" -ne 1 ]; then
    echo "$USAGE"
    exit 1
fi

ldap_passwd=$1

ldapadd -w $ldap_passwd -x -D "cn=sgiamadmin,dc=seagate,dc=com" -f /opt/seagate/s3/install/ldap/background_delete_account.ldif -H ldapi:///
