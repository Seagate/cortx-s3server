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
usage() { echo "Usage: [-p <ldap admin password>]" 1>&2; exit 1; }

while getopts ":p:" o; do
    case "${o}" in
        p)
            password=${OPTARG}
            ;;
        *)
            usage
            ;;
    esac
done
shift $((OPTIND-1))

if [ -z ${password} ]
then
    usage
    exit 1
fi
host_list=/opt/seagate/cortx/s3/install/ldap/replication/host_list
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
id=1
#Below will generate serverid from host list provided
getServerIdFromHostFile()
{
    while read host; do
        if [ "$host" == "$HOSTNAME" ]
        then
            break
        fi
    id=`expr ${id} + 1`
    done <$host_list
}
#Below will get serverid from salt command
getServerIdWithSalt()
{
    nodeId=$(salt-call grains.get id --output=newline_values_only)
    IFS='-'
    read -ra ID <<< "$nodeId"
    id=${ID[1]}
}


#olcServerId script
checkHostValidity
if hash salt 2>/dev/null; then
    getServerIdWithSalt
else
    getServerIdFromHostFile
fi
sed -e "s/\${serverid}/$id/" revertServerIdTemplate.ldif > scriptRevertServerId.ldif
ldapmodify -Y EXTERNAL  -H ldapi:/// -f scriptRevertServerId.ldif
rm scriptRevertServerId.ldif

rm -rf /etc/openldap/slapd.d/cn\=config/cn\=module{2}.ldif
rm -rf /etc/openldap/slapd.d/cn\=config/olcDatabase\={2}mdb/olcOverlay\={2}syncprov.ldif

#update replicaiton config

rid=1
while read host; do
  sed -e "s/\${rid}/$rid/" -e "s/\${provider}/$host/" -e "s/\${credentials}/$password/" revertConfigTemplate.ldif > revertScriptConfig.ldif
  ldapmodify -Y EXTERNAL  -H ldapi:/// -f revertScriptConfig.ldif
  rm revertScriptConfig.ldif
  rid=`expr ${rid} + 1`
done <$host_list
ldapmodify -Y EXTERNAL  -H ldapi:/// -f revertMirrorModeConfig.ldif
iteration=1
# Update mdb file
while read host; do
  sed -e "s/\${rid}/$rid/" -e "s/\${provider}/$host/" -e "s/\${credentials}/$password/" revertDataTemplate.ldif > revertScriptData.ldif
  ldapmodify -Y EXTERNAL  -H ldapi:/// -f revertScriptData.ldif
  rm revertScriptData.ldif
  rid=`expr ${rid} + 1`
  iteration=`expr ${iteration} + 1`
done <$host_list
ldapmodify -Y EXTERNAL  -H ldapi:/// -f revertMirrorModeData.ldif
rm $host_list
systemctl restart slapd
