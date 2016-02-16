#!/bin/sh -x

# Delete all the entries from LDAP.
ldapdelete -x -w seagate -D "cn=admin,dc=seagate,dc=com" -r "dc=seagate,dc=com"

# Delete the schema from LDAP.
rm /etc/openldap/slapd.d/cn\=config/cn\=schema/cn\=\{1\}s3user.ldif

# Restart slapd
systemctl restart slapd

# Add the new schema.
ldapadd -x -w seagate -D "cn=admin,cn=config" -f cn\=\{1\}s3user.ldif

# Restart slapd to update the changes.
systemctl restart slapd

# Initialize ldap
ldapadd -w seagate -x -D "cn=admin,dc=seagate,dc=com" -f ldap-init.ldif
