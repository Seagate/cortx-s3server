#!/bin/bash -e
#This script can be used to update encrpted password in authserver.properties
#file. this performs below steps
# 1. Encrypts given password using /opt/seagate/auth/tools/AuthPassEncryptCLI
# 2. Updates encrypted password in /opt/seagate/auth/resources/authserver.properites file

USAGE="USAGE: bash $(basename "$0") [-ldappasswd <passwd>] [--help | -h]
Encrypt and update ldap passwd
where:
-ldappasswd <passwd>   Update Auth server with given ldap passwd
--help              display this help and exit"
if [ -z $1 ]
  then
    echo "$USAGE"
    exit 1
fi

defaultpasswd=false
case "$1" in
    -ldappasswd )
        if [ -z $2 ]
         then
           echo "$USAGE"
           exit 1
        fi
        passwd=$2
        encryptpass=`java -jar /opt/seagate/auth/AuthPassEncryptCLI-1.0-0.jar -s $passwd`
        echo $encryptpass
        sed -i "s/^ldapLoginPW=.*/ldapLoginPW=${encryptpass}/" /opt/seagate/auth/resources/authserver.properties
        ;;
    --help | -h )
        echo "$USAGE"
        exit 1
        ;;
esac
