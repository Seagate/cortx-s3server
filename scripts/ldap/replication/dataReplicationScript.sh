#!/bin/sh

set -x

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

sed -e "s/\${serverid}/$id/" serverIdTemplate.ldif > scriptServerId.ldif
ldapmodify -Y EXTERNAL  -H ldapi:/// -f scriptServerId.ldif
rm scriptServerId.ldif

ldapadd -Y EXTERNAL -H ldapi:/// -f syncprov_mod.ldif

ldapadd -Y EXTERNAL -H ldapi:/// -f syncprov.ldif

teration=1
# Update mdb file
while read host; do
sed -e "s/\${rid}/$rid/" -e "s/\${provider}/$host/" -e "s/\${credentials}/$password/" dataTemplate.ldif > scriptData.ldif
if [ ${iteration} -eq 2 ] && [ ${id} -eq 1 ]
then
    echo "-" >> scriptData.ldif
    echo "add: olcMirrorMode" >> scriptData.ldif
    echo "olcMirrorMode: TRUE" >> scriptData.ldif
fi
if [ ${iteration} -eq 1 ] && [ ${id} -ne 1 ]
then
    echo "-" >> scriptData.ldif
    echo "add: olcMirrorMode" >> scriptData.ldif
    echo "olcMirrorMode: TRUE" >> scriptData.ldif
fi
ldapmodify -Y EXTERNAL  -H ldapi:/// -f scriptData.ldif
rm scriptData.ldif
rid=`expr ${rid} + 1`
iteration=`expr ${iteration} + 1`
done <$host_list
