#!/bin/sh

USAGE="USAGE: create_s3_recovery_tool_account.sh <ldap passwd> "

if [ "$#" -ne 1 ]; then
    echo "$USAGE"
    exit 1
fi

ldap_passwd=$1

ldapadd -w $ldap_passwd -x -D "cn=sgiamadmin,dc=seagate,dc=com" -f /opt/seagate/cortx/s3/install/ldap/s3_recovery_tool_account.ldif -H ldapi:///
