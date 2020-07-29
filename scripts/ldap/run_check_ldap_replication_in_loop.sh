#!/bin/bash
a=0
while true
do
        ./check_ldap_replication.sh -s hostname.txt -p ldapadmin
        a=$(( $a + 1 ))
        if [ $a -eq 500 ]
        then
                break
        fi
done
