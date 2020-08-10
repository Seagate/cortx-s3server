#!/bin/sh
#
# Copyright (c) 2020 Seagate Technology LLC and/or its Affiliates
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# For any questions about this software or licensing,
# please email opensource@seagate.com or cortx-questions@seagate.com.
#


# You should provide Host_list in a file,and give this file as a argument to script.say there are three nodes in cluster with A,B and c.file that contains this hosts should look like below.
# suppose file created is hostlist.txt, cat hostlist.txt should be
# hostname -f  of A
# hostname -f of B
# hostname -f of c

cluster_replication=true

usage() { echo "Usage: [-s <provide file containing hostnames of nodes in cluster>],[-p <ldap password>]" 1>&2; exit 1; }

while getopts ":s:p:" o; do
    case "${o}" in
        s)
            host_list=${OPTARG}
            ;;
        p)
            ldappasswd=${OPTARG}
            ;;
        *)
            usage
            ;;
    esac
done
shift $((OPTIND-1))

if [ -z ${host_list} ]
then
    usage
    exit 1
fi

if [ ! -s "$host_list" ]
then
  echo "file $host_list is empty"
  exit 1
fi

echo "Check Ldap replication for below nodes"

while read p; do
  echo "$p"
done <$host_list

#add account

node1=$(head -n 1 $host_list)

ldapadd -w $ldappasswd -x -D "cn=sgiamadmin,dc=seagate,dc=com" -f create_replication_account.ldif  -H ldap://$node1 || exit 0

#adding some delay for successful replication

sleep 1s

# check replication on node 2
while read node; do
  output=$(ldapsearch -b "o=sanity-test-repl-account,ou=accounts,dc=s3,dc=seagate,dc=com" -x -w $ldappasswd -D "cn=sgiamadmin,dc=seagate,dc=com" -H ldap://$node) || echo "failed to search"
  if [[ $output == *"No such object"* ]]
  then
    cluster_replication=false
    echo "Replication is not setup properly on node $node"
  else
    echo "Replication is setup properly on node $node"
  fi
done <$host_list

if [ "$cluster_replication" = true ]
then
  echo "Replication is setup properly on cluster"
else
   echo "Setup replication on nodes,which are not configured correctly for replication"
fi

#delete account created

ldapdelete -x -w $ldappasswd -r "o=sanity-test-repl-account,ou=accounts,dc=s3,dc=seagate,dc=com" -D "cn=sgiamadmin,dc=seagate,dc=com" -H ldap://$node1 || exit 1


##################create IAM users and their respective accesskeys testing ####################

ldapadd -w seagate -x -D "cn=admin,dc=seagate,dc=com" -f test_data.ldif

sleep 1s

# check replication on other nodes
while read node; do
  output=$(ldapsearch -b "ak=CKIAJTYX16YCKQSAJT8Q,ou=accesskeys,dc=s3,dc=seagate,dc=com" -x -w $ldappasswd -D "cn=sgiamadmin,dc=seagate,dc=com" -H ldap://$node) || echo "failed to search"
  if [[ $output == *"No such object"* ]]
  then
    cluster_replication=false
    echo "Replication is not setup properly on node $node"
  else
    echo "Replication is setup properly on node $node"
  fi
done <$host_list

if [ "$cluster_replication" = true ]
then
  echo "Replication is setup properly on cluster"
else
   echo "Setup replication on nodes,which are not configured correctly for replication"
fi

#delete created account and accesskeys

ldapdelete -x -w $ldappasswd -r "o=s3_test3,ou=accounts,dc=s3,dc=seagate,dc=com" -D "cn=sgiamadmin,dc=seagate,dc=com" -H ldapi:///
ldapdelete -x -w $ldappasswd -r "ak=AKIAJPINPFR1TPAYOGPA,ou=accesskeys,dc=s3,dc=seagate,dc=com" -D "cn=sgiamadmin,dc=seagate,dc=com" -H ldapi:///
ldapdelete -x -w $ldappasswd -r "ak=AKIAJTYX16YCKQSAJT8Q,ou=accesskeys,dc=s3,dc=seagate,dc=com" -D "cn=sgiamadmin,dc=seagate,dc=com" -H ldapi:///
ldapdelete -x -w $ldappasswd -r "ak=BKIAJTYX16YCKQSAJT8Q,ou=accesskeys,dc=s3,dc=seagate,dc=com" -D "cn=sgiamadmin,dc=seagate,dc=com" -H ldapi://
ldapdelete -x -w $ldappasswd -r "ak=CKIAJTYX16YCKQSAJT8Q,ou=accesskeys,dc=s3,dc=seagate,dc=com" -D "cn=sgiamadmin,dc=seagate,dc=com" -H ldapi://
