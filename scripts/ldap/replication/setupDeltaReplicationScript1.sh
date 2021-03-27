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
# Configure ldap-replication
##################################
usage() { echo "Usage: [-h <file containing hostnames of nodes in cluster>] [-p <rootDN password>]" 1>&2; exit 1; }

while getopts ":h:p:" o; do
    case "${o}" in
        h)
            host_list=${OPTARG}
            ;;
        p)
            password=${OPTARG}
            ;;
        *)
            usage
            ;;
    esac
done
shift "$((OPTIND-1))"

if [ -z ${host_list} ] || [ -z ${password} ]
then
    usage
    exit 1
fi

INSTALLDIR="/opt/seagate/cortx/s3/install/ldap/replication"

# checkHostValidity will check if all provided hosts are valid and reachable
checkHostValidity()
{
    while read host; do
        isValid=$(ping -c 1 ${host} | grep bytes | wc -l)
        if [ "$isValid" -le 1 ]
        then
            echo "ERROR: $host is either invalid or not reachable."
            exit
        else
            echo "INFO: $host is valid and reachable."
        fi
    done < "$host_list"
}

# Check if hosts are valid
checkHostValidity

# getServerIdFromHostFile will generate serverid from host list provided
id=1
getServerIdFromHostFile()
{
    while read host; do
        if [ "$host" == "$HOSTNAME" ]
        then
            break
        fi
    id=$(expr ${id} + 1)
    done < "$host_list"
}

# update serverID
getServerIdFromHostFile

sed -e "s/\${serverid}/$id/" $INSTALLDIR/serverIdTemplate.ldif > scriptServerId.ldif
ldapmodify -Y EXTERNAL  -H ldapi:/// -f scriptServerId.ldif
rm scriptServerId.ldif

ldapadd -Y EXTERNAL -H ldapi:/// -f $INSTALLDIR/syncprov_delta.ldif
ldapadd -Y EXTERNAL -H ldapi:/// -f $INSTALLDIR/index_delta.ldif

# update replication config
echo "dn: olcDatabase={2}mdb,cn=config" > scriptConfig.ldif
echo "changetype: modify" >> scriptConfig.ldif
echo "add: olcSyncRepl" >> scriptConfig.ldif
rid=1
while read host; do
    if [ "$host" != "$HOSTNAME" ]
    then
        sed -e "s/\${rid}/$rid/" -e "s/\${provider}/$host/" -e "s/\${credentials}/$password/" $INSTALLDIR/deltaConfigTemplate.ldif >> scriptConfig.ldif
        rid=`expr ${rid} + 1`
    fi
done <$host_list

echo "-" >> scriptConfig.ldif
echo "add: olcMirrorMode" >> scriptConfig.ldif
echo "olcMirrorMode: TRUE" >> scriptConfig.ldif

ldapmodify -Y EXTERNAL  -H ldapi:/// -f scriptConfig.ldif
rm scriptConfig.ldif
