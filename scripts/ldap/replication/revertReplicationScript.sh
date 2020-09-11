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
# Configure replication 
##################################
usage() { echo "Usage: [-h <provide file containing hostnames of nodes in cluster>],[-p <ldap admin password>]" 1>&2; exit 1; }

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
shift $((OPTIND-1))

if [ -z ${host_list} ] || [ -z ${password} ]
then
    usage
    exit 1
fi

#Below function will check if all provided hosts are valid or not
checkHostValidity()
{
    while read host; do
        isValid=`ping -c 1 ${host} | grep bytes | wc -l`
        if [ "$isValid" -le 1 ]
        then
            echo ${host}" is either invalid or not reachable. Please check or correct your entry in host file"
            exit
        fi
    done <$host_list
}

checkHostValidity
#revert serverid
result=$(ldapsearch -w seagate -x -D cn=admin,cn=config -b cn=config | grep olcServerID:)
IFS=' '
read -ra serverId <<< "$result"
id=${serverId[1]}
sed -e "s/\${serverid}/$id/" revertServerIdTemplate.ldif > scriptRevertServerId.ldif
ldapmodify -Y EXTERNAL  -H ldapi:/// -f scriptRevertServerId.ldif
rm scriptRevertServerId.ldif

rm -rf /etc/openldap/slapd.d/cn\=config/cn\=module{2}.ldif
rm -rf /etc/openldap/slapd.d/cn\=config/olcDatabase\={2}mdb/olcOverlay\={2}syncprov.ldif

#revert replication config
while read host; do
record=1
ldapsearch -w seagate -x -D cn=admin,cn=config -b cn=config | grep olcSyncrepl | grep "${host}"  |  while read -r line ; do
IFS=' '
read -ra rid <<< "$line"
ridLine=${rid[1]}
IFS='='
read -ra ridline <<< "$ridLine"
ridNumber=${ridline[1]}
if [ $record -eq 1 ]
then
  sed -e "s/\${rid}/$ridNumber/" -e "s/\${provider}/$host/" -e "s/\${credentials}/$password/" revertConfigTemplate.ldif > revertScriptConfig.ldif
  ldapmodify -Y EXTERNAL  -H ldapi:/// -f revertScriptConfig.ldif
  rm revertScriptConfig.ldif
else
sed -e "s/\${rid}/$ridNumber/" -e "s/\${provider}/$host/" -e "s/\${credentials}/$password/" revertDataTemplate.ldif > revertScriptData.ldif
  ldapmodify -Y EXTERNAL  -H ldapi:/// -f revertScriptData.ldif
  rm revertScriptData.ldif
fi
record=`expr ${record} + 1`
done
done <$host_list
ldapmodify -Y EXTERNAL  -H ldapi:/// -f revertMirrorModeConfig.ldif
ldapmodify -Y EXTERNAL  -H ldapi:/// -f revertMirrorModeData.ldif

systemctl restart slapd
