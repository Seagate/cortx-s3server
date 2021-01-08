#!/bin/sh

die_with_error () {
        echo $1
        exit -1
}

# Attempt ldap clean up since ansible openldap setup is not idempotent
systemctl stop slapd 2>/dev/null || die_with_error "slapd could not be stopped!"
yum remove -y openldap-servers openldap-clients || "openldap-servers openldap-clients removal failed"

rm -f /etc/openldap/slapd.d/cn\=config/cn\=schema/cn\=\{1\}s3user.ldif
rm -rf /var/lib/ldap/*
rm -f /etc/sysconfig/slapd* 2>/dev/null || /bin/true
rm -f /etc/openldap/slapd* 2>/dev/null || /bin/true
rm -rf /etc/openldap/slapd.d/*
