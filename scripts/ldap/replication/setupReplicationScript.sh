#!/bin/bash -e
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
##################################
# Configure ldap replication
##################################

usage() { echo "Usage: [-h comma-separated list of hostnames of nodes in the cluster] [-p ldap admin password]" >&2 }

while getopts ":h:p:" OPTION; do
    case "$OPTION" in
        h)
            host_list="$OPTARG"
            ;;
        p)
            password="$OPTARG"
            ;;
        ?)
            usage
            exit 1
            ;;
    esac
done
shift "$((OPTIND-1))"

if [ -z "$host_list" ] || [ -z "$password" ]
then
    usage
    exit 1
fi

INSTALLDIR="/opt/seagate/cortx/s3/install/ldap/replication"

# checkHostValidity will check if all provided hosts are valid and reachable
checkHostValidity()
{
    # read comma-separated string: 'host_list' in an array
    while IFS=',' read -ra hosts_arr
    do
        for host in "${hosts_arr[@]}"
        do
            isValid=$(ping -c 1 "$host" | grep bytes | wc -l)
            if [ "$isValid" -le 1 ]
            then
                echo "$host is either invalid or not reachable."
                exit 1
            fi
        done
    done <<< "$host_list"
}

# check if hosts are valid
checkHostValidity

# getServerIdFromHostFile will generate serverid from host list provided
id=1
getServerIdFromHostFile()
{
    # read comma-separated string: 'host_list' in an array
    IFS=','
    read -ra hosts_arr <<< "$host_list"
    for host in "${hosts_arr[@]}"
    do
        if [ "$host" == "$HOSTNAME" ]
        then
            break
        fi
    id=$(expr ${id} + 1)
    done
}

# update serverID
getServerIdFromHostFile

sed -e "s/\${serverid}/$id/" $INSTALLDIR/serverIdTemplate.ldif > scriptServerId.ldif
ldapmodify -Y EXTERNAL  -H ldapi:/// -f scriptServerId.ldif
rm scriptServerId.ldif

ldapadd -Y EXTERNAL -H ldapi:/// -f $INSTALLDIR/syncprov_mod.ldif
ldapadd -Y EXTERNAL -H ldapi:/// -f $INSTALLDIR/syncprov.ldif

# update replication config
echo "dn: olcDatabase={0}config,cn=config" > scriptConfig.ldif
echo "changetype: modify" >> scriptConfig.ldif
echo "add: olcSyncRepl" >> scriptConfig.ldif
rid=1
while IFS=',' read -ra hosts_arr
do
    for host in "${hosts_arr[@]}"
    do
        sed -e "s/\${rid}/$rid/" -e "s/\${provider}/$host/" -e "s/\${credentials}/$password/" $INSTALLDIR/configTemplate.ldif >> scriptConfig.ldif
        rid=$(expr ${rid} + 1)
    done
done <<< "$host_list"

echo "-" >> scriptConfig.ldif
echo "add: olcMirrorMode" >> scriptConfig.ldif
echo "olcMirrorMode: TRUE" >> scriptConfig.ldif

ldapmodify -Y EXTERNAL  -H ldapi:/// -f scriptConfig.ldif
rm scriptConfig.ldif

# Update mdb file
echo "dn: olcDatabase={2}mdb,cn=config" > scriptData.ldif
echo "changetype: modify" >> scriptData.ldif
echo "add: olcSyncRepl" >> scriptData.ldif

while IFS=',' read -ra hosts_arr
do
    for host in "${hosts_arr[@]}"
    do
        sed -e "s/\${rid}/$rid/" -e "s/\${provider}/$host/" -e "s/\${credentials}/$password/" $INSTALLDIR/dataTemplate.ldif >> scriptData.ldif
        rid=$(expr ${rid} + 1)
    done
done <<< "$host_list"

echo "-" >> scriptData.ldif
echo "add: olcMirrorMode" >> scriptData.ldif
echo "olcMirrorMode: TRUE" >> scriptData.ldif

ldapmodify -Y EXTERNAL  -H ldapi:/// -f scriptData.ldif
rm scriptData.ldif
