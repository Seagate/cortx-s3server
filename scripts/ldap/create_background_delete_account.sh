#!/bin/sh

ldapadd -w seagate -x -D "cn=admin,dc=seagate,dc=com" -f background_delete_account.ldif
