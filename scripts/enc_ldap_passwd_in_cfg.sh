#!/bin/sh

set -e

# This script can be used to update encrpted password in authserver.properties
# file. Performs below steps
# 1. Encrypts given password using /opt/seagate/auth/AuthPassEncryptCLI-1.0-0.jar
# 2. Updates encrypted password in given authserver.properites file

USAGE="USAGE: bash $(basename "$0") -l <ldap passwd>
          -p <authserver.properties file path> [-h]

Encrypt ldap passwd and update in given authserver.properties file.
"

if [ "$#" -ne 4 ]; then
    echo "$USAGE"
    exit 1
fi

ldap_passwd=
auth_properties=
encrypt_cli=/opt/seagate/auth/AuthPassEncryptCLI-1.0-0.jar

# read the options
while getopts ":l:p:h:" o; do
    case "${o}" in
        l)
            ldap_passwd=${OPTARG}
            ;;
        p)
            auth_properties=${OPTARG}
            ;;
        *)
            echo "$USAGE"
            exit 0
            ;;
    esac
done
shift $((OPTIND-1))

if [ ! -e $encrypt_cli ]; then
    echo "$encrypt_cli does not exists."
    exit 1
fi

if [ ! -e $auth_properties ]; then
    echo "$auth_properties file does not exists."
    exit 1
fi

if [ -z $ldap_passwd ]; then
    echo "Ldap password not specified."
    echo "$USAGE"
    exit 1
fi

# Encrypt the password
encrypted_pass=`java -jar $encrypt_cli -s $ldap_passwd`

# Update the config
escaped_pass=`echo "$encrypted_pass" | sed 's/\//\\\\\\//g'`
sed -i "s/^ldapLoginPW=.*/ldapLoginPW=${escaped_pass}/" $auth_properties
