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
# Cleanup ldap-replication
##################################
usage() { echo "Usage: [-p <rootDN password>] " 1>&2; exit 1; }

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
shift "$((OPTIND-1))"

if [ -z ${password} ]
then
    usage
    exit 1
fi

CLEANUPDIR="/opt/seagate/cortx/s3/install/ldap/replication/cleanup"

op=$(ldapsearch -w $password -x -D cn=admin,cn=config -b cn=config "olcServerID")
if [[ $op == *"olcServerID:"* ]];then
  ldapmodify -Y EXTERNAL  -H ldapi:/// -f $CLEANUPDIR/olcserverid.ldif
  echo "olcserverid is cleaned up"
fi

op=$(ldapsearch -w $password -x -D cn=admin,cn=config -b olcDatabase={0}config,cn=config "olcSyncrepl")
if [[ $op == *"olcSyncrepl:"* ]];then
  ldapmodify -Y EXTERNAL  -H ldapi:/// -f $CLEANUPDIR/config.ldif
  echo "olcsynrepl at config level is cleaned up"
fi

op=$(ldapsearch -w $password -x -D cn=admin,cn=config -b olcDatabase={0}config,cn=config "olcMirrorMode")
if [[ $op == *"olcMirrorMode:"* ]];then
  ldapmodify -Y EXTERNAL  -H ldapi:/// -f $CLEANUPDIR/olcmirromode_config.ldif
  echo "olcmirrormode at config level is cleaned up"
fi

op=$(ldapsearch -w $password -x -D cn=admin,cn=config -b olcDatabase={2}mdb,cn=config "olcSyncrepl")
if [[ $op == *"olcSyncrepl:"* ]];then
  ldapmodify -Y EXTERNAL  -H ldapi:/// -f $CLEANUPDIR/data.ldif
  echo "olcsynrepl at data level is cleaned up"
fi

op=$(ldapsearch -w $password -x -D cn=admin,cn=config -b olcDatabase={2}mdb,cn=config "olcMirrorMode")
if [[ $op == *"olcMirrorMode:"* ]];then
  ldapmodify -Y EXTERNAL  -H ldapi:/// -f $CLEANUPDIR/olcmirromode_data.ldif
  echo "olcmirrormode at data level is cleaned up"
fi
echo "ran"
