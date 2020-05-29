#!/bin/sh
USAGE="USAGE: delete_background_delete_account.sh <ldap passwd> "

if [ "$#" -ne 1 ]; then
    echo "$USAGE"
    exit 1
fi

ldap_passwd=$1


ldapdelete -x -w $ldap_passwd -r "ak=AKIAJPINPFRBTPAYXAHZ,ou=accesskeys,dc=s3,dc=seagate,dc=com" -D "cn=sgiamadmin,dc=seagate,dc=com" -H ldapi:///

ldapdelete -x -w $ldap_passwd -r "o=s3-background-delete-svc,ou=accounts,dc=s3,dc=seagate,dc=com" -D "cn=sgiamadmin,dc=seagate,dc=com" -H ldapi:///

