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

usage() { echo "Usage: [-s <ssh password>],[-h <provide file containing hostnames of nodes in cluster>],[-c <current hostname>],[-n <total existing cluster nodes>],[-p <ldap admin password>]" 1>&3; exit 1; }

while getopts ":s:h:c:n:p:" o; do
    case "${o}" in
        s)
            ssh_passwd=${OPTARG}
            ;;
        h)
            host_list=${OPTARG}
            ;;
        c)
            current_host=${OPTARG}
            ;;
        n)
            total_nodes=${OPTARG}
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

if [ -z ${current_host} ] || [ -z ${total_nodes} ] || [ -z ${host_list} ] || [ -z ${password} ]
then
    usage
    exit 1
fi


#create,run and delete olcServerId script
serverId=`expr $total_nodes + 1`
echo "dn: cn=config" >> scriptServerId.ldif
echo "changetype: modify" >> scriptServerId.ldif
echo "add: olcServerID" >> scriptServerId.ldif
finalLine="olcServerID: "${serverId}
echo ${finalLine} >> scriptServerId.ldif
ldapmodify -Y EXTERNAL  -H ldapi:/// -f scriptServerId.ldif
rm scriptServerId.ldif

ldapadd -Y EXTERNAL -H ldapi:/// -f syncprov_mod.ldif

ldapadd -Y EXTERNAL -H ldapi:/// -f syncprov.ldif


IFS2=${IFS}
IFS=
echo "dn: olcDatabase={0}config,cn=config" >> scriptConfig.ldif
echo "changetype: modify" >> scriptConfig.ldif
echo "add: olcSyncRepl" >> scriptConfig.ldif
#Add all entries
rid=1
while read host; do
ridLine="olcSyncRepl: rid=00"${rid}
echo ${ridLine} >> scriptConfig.ldif
providerLine="  provider=ldap://"${host}":389/"
echo ${providerLine} >> scriptConfig.ldif
echo "  bindmethod=simple" >> scriptConfig.ldif
echo "  binddn=\"cn=admin,cn=config\"" >> scriptConfig.ldif
passwordLine="  credentials="${password}
echo ${passwordLine} >> scriptConfig.ldif
echo "  searchbase=\"cn=config\"" >> scriptConfig.ldif
echo "  scope=sub" >> scriptConfig.ldif
echo "  schemachecking=on" >> scriptConfig.ldif
echo "  type=refreshAndPersist" >> scriptConfig.ldif
echo "  retry=\"30 5 300 3\"" >> scriptConfig.ldif
echo "  interval=00:00:05:00" >> scriptConfig.ldif
rid=`expr ${rid} + 1`
done <$host_list
#Add current host
ridLine="olcSyncRepl: rid=00"${rid}
rid=`expr ${rid} + 1`
echo ${ridLine} >> scriptConfig.ldif
providerLine="  provider=ldap://"${current_host}":389/"
echo ${providerLine} >> scriptConfig.ldif
echo "  bindmethod=simple" >> scriptConfig.ldif
echo "  binddn=\"cn=admin,cn=config\"" >> scriptConfig.ldif
passwordLine="  credentials="${password}
echo ${passwordLine} >> scriptConfig.ldif
echo "  searchbase=\"cn=config\"" >> scriptConfig.ldif
echo "  scope=sub" >> scriptConfig.ldif
echo "  schemachecking=on" >> scriptConfig.ldif
echo "  type=refreshAndPersist" >> scriptConfig.ldif
echo "  retry=\"30 5 300 3\"" >> scriptConfig.ldif
echo "  interval=00:00:05:00" >> scriptConfig.ldif
if [ ${total_nodes} -gt 0 ]
then
echo "-" >> scriptConfig.ldif
echo "add: olcMirrorMode" >> scriptConfig.ldif
echo "olcMirrorMode: TRUE" >> scriptConfig.ldif
fi
ldapmodify -Y EXTERNAL  -H ldapi:/// -f scriptConfig.ldif
rm scriptConfig.ldif

#Add all host entries
echo "dn: olcDatabase={2}mdb,cn=config" >> scriptData.ldif
echo "changetype: modify" >> scriptData.ldif
echo "add: olcSyncRepl" >> scriptData.ldif

while read host; do
ridLine="olcSyncRepl: rid=00"${rid}
echo ${ridLine} >> scriptData.ldif
providerLine="  provider=ldap://"${host}":389/"
echo ${providerLine} >> scriptData.ldif
echo "  bindmethod=simple" >> scriptData.ldif
echo "  binddn=\"cn=admin,dc=seagate,dc=com\"" >> scriptData.ldif
passwordLine="  credentials="${password}
echo ${passwordLine} >> scriptData.ldif
echo "  searchbase=\"dc=seagate,dc=com\"" >> scriptData.ldif
echo "  scope=sub" >> scriptData.ldif
echo "  schemachecking=on" >> scriptData.ldif
echo "  type=refreshAndPersist" >> scriptData.ldif
echo "  retry=\"30 5 300 3\"" >> scriptData.ldif
echo "  interval=00:00:05:00" >> scriptData.ldif
rid=`expr ${rid} + 1`
done <$host_list
#Add current host
ridLine="olcSyncRepl: rid=00"${rid}
rid=`expr ${rid} + 1`
echo ${ridLine} >> scriptData.ldif
providerLine="  provider=ldap://"${current_host}":389/"
echo ${providerLine} >> scriptData.ldif
echo "  bindmethod=simple" >> scriptData.ldif
echo "  binddn=\"cn=admin,dc=seagate,dc=com\"" >> scriptData.ldif
passwordLine="  credentials="${password}
echo ${passwordLine} >> scriptData.ldif
echo "  searchbase=\"dc=seagate,dc=com\"" >> scriptData.ldif
echo "  scope=sub" >> scriptData.ldif
echo "  schemachecking=on" >> scriptData.ldif
echo "  type=refreshAndPersist" >> scriptData.ldif
echo "  retry=\"30 5 300 3\"" >> scriptData.ldif
echo "  interval=00:00:05:00" >> scriptData.ldif


if [ ${total_nodes} -gt 0 ]
then
echo "-" >> scriptData.ldif
echo "add: olcMirrorMode" >> scriptData.ldif
echo "olcMirrorMode: TRUE" >> scriptData.ldif
fi

ldapmodify -Y EXTERNAL  -H ldapi:/// -f scriptData.ldif
rm scriptData.ldif


#ssh remotely and add current host in olcDatabase config
while read host; do
echo $host
sshpass -p ${ssh_passwd} ssh-copy-id "root@"${host}
echo "dn: olcDatabase={0}config,cn=config" | sshpass -p ${ssh_passwd} ssh "root@"${host} -T "cat >> scriptCurrentHost.ldif"
echo "changetype: modify" | sshpass -p ${ssh_passwd} ssh "root@"${host} -T "cat >> scriptCurrentHost.ldif"
echo "add: olcSyncRepl" | sshpass -p ${ssh_passwd} ssh "root@"${host} -T "cat >> scriptCurrentHost.ldif"
ridLine="olcSyncRepl: rid=00"${serverId}
echo ${ridLine} | sshpass -p ${ssh_passwd} ssh "root@"${host} -T "cat >> scriptCurrentHost.ldif"
providerLine="  provider=ldap://"${current_host}":389/"
echo ${providerLine} | sshpass -p ${ssh_passwd} ssh "root@"${host} -T "cat >> scriptCurrentHost.ldif"
echo "  bindmethod=simple" | sshpass -p ${ssh_passwd} ssh "root@"${host} -T "cat >> scriptCurrentHost.ldif"
echo "  binddn=\"cn=admin,cn=config\"" | sshpass -p ${ssh_passwd} ssh "root@"${host} -T "cat >> scriptCurrentHost.ldif"
passwordLine="  credentials="${password}
echo ${passwordLine} | sshpass -p ${ssh_passwd} ssh "root@"${host} -T "cat >> scriptCurrentHost.ldif"
echo "  searchbase=\"cn=config\"" | sshpass -p ${ssh_passwd} ssh "root@"${host} -T "cat >> scriptCurrentHost.ldif"
echo "  scope=sub" | sshpass -p ${ssh_passwd} ssh "root@"${host} -T "cat >> scriptCurrentHost.ldif"
echo "  schemachecking=on" | sshpass -p ${ssh_passwd} ssh "root@"${host} -T "cat >> scriptCurrentHost.ldif"
echo "  type=refreshAndPersist" | sshpass -p ${ssh_passwd} ssh "root@"${host} -T "cat >> scriptCurrentHost.ldif"
echo "  retry=\"30 5 300 3\"" | sshpass -p ${ssh_passwd} ssh "root@"${host} -T "cat >> scriptCurrentHost.ldif"
echo "  interval=00:00:05:00" | sshpass -p ${ssh_passwd} ssh "root@"${host} -T "cat >> scriptCurrentHost.ldif"
if [ ${total_nodes} -eq 1 ]
then
echo "-" | sshpass -p ${ssh_passwd} ssh "root@"${host} -T "cat >> scriptCurrentHost.ldif"
echo "add: olcMirrorMode" | sshpass -p ${ssh_passwd} ssh "root@"${host} -T "cat >> scriptCurrentHost.ldif"
echo "olcMirrorMode: TRUE" | sshpass -p ${ssh_passwd} ssh "root@"${host} -T "cat >> scriptCurrentHost.ldif"
fi
command="ldapmodify -Y EXTERNAL  -H ldapi:/// -f scriptCurrentHost.ldif"
sshpass -p ${ssh_passwd} ssh -n root@${host} "${command}"
removeCommand="rm scriptCurrentHost.ldif"
sshpass -p ${ssh_passwd} ssh -n root@${host} "${removeCommand}"
done<$host_list

#ssh remotely and add current host in olcDatabase={2}mdb
while read host; do
echo $host
echo "dn: olcDatabase={2}mdb,cn=config" | sshpass -p ${ssh_passwd} ssh "root@"${host} -T "cat >> scriptCurrentHost.ldif"
echo "changetype: modify" | sshpass -p ${ssh_passwd} ssh "root@"${host} -T "cat >> scriptCurrentHost.ldif"
echo "add: olcSyncRepl" | sshpass -p ${ssh_passwd} ssh "root@"${host} -T "cat >> scriptCurrentHost.ldif"
ridNew=`expr ${serverId} + 1`
ridLine="olcSyncRepl: rid=00"${ridNew}
echo ${ridLine} | sshpass -p ${ssh_passwd} ssh "root@"${host} -T "cat >> scriptCurrentHost.ldif"
providerLine="  provider=ldap://"${current_host}":389/"
echo ${providerLine} | sshpass -p ${ssh_passwd} ssh "root@"${host} -T "cat >> scriptCurrentHost.ldif"
echo "  bindmethod=simple" | sshpass -p ${ssh_passwd} ssh "root@"${host} -T "cat >> scriptCurrentHost.ldif"
echo "  binddn=\"cn=admin,dc=seagate,dc=com\"" | sshpass -p ${ssh_passwd} ssh "root@"${host} -T "cat >> scriptCurrentHost.ldif"
passwordLine="  credentials="${password}
echo ${passwordLine} | sshpass -p ${ssh_passwd} ssh "root@"${host} -T "cat >> scriptCurrentHost.ldif"
echo "  searchbase=\"dc=seagate,dc=com\"" | sshpass -p ${ssh_passwd} ssh "root@"${host} -T "cat >> scriptCurrentHost.ldif"
echo "  scope=sub" | sshpass -p ${ssh_passwd} ssh "root@"${host} -T "cat >> scriptCurrentHost.ldif"
echo "  schemachecking=on" | sshpass -p ${ssh_passwd} ssh "root@"${host} -T "cat >> scriptCurrentHost.ldif"
echo "  type=refreshAndPersist" | sshpass -p ${ssh_passwd} ssh "root@"${host} -T "cat >> scriptCurrentHost.ldif"
echo "  retry=\"30 5 300 3\"" | sshpass -p ${ssh_passwd} ssh "root@"${host} -T "cat >> scriptCurrentHost.ldif"
echo "  interval=00:00:05:00" | sshpass -p ${ssh_passwd} ssh "root@"${host} -T "cat >> scriptCurrentHost.ldif"
if [ ${total_nodes} -eq 1 ]
then
echo "-" | sshpass -p ${ssh_passwd} ssh "root@"${host} -T "cat >> scriptCurrentHost.ldif"
echo "add: olcMirrorMode" | sshpass -p ${ssh_passwd} ssh "root@"${host} -T "cat >> scriptCurrentHost.ldif"
echo "olcMirrorMode: TRUE" | sshpass -p ${ssh_passwd} ssh "root@"${host} -T "cat >> scriptCurrentHost.ldif"
fi
command="ldapmodify -Y EXTERNAL  -H ldapi:/// -f scriptCurrentHost.ldif"
sshpass -p ${ssh_passwd} ssh -n root@${host} "${command}"
removeCommand="rm scriptCurrentHost.ldif"
sshpass -p ${ssh_passwd} ssh -n root@${host} "${removeCommand}"
done<$host_list

IFS=${IFS2}
