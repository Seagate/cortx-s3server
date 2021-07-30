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


SCRIPT_PATH=$(readlink -f "$0")
BASEDIR=$(dirname "$SCRIPT_PATH")
usage() {
  echo 'Usage: runallsystest.sh { --no_https | --use_ipv6 | --help | -h}'
  echo 'Parameters as below :                                '
  echo '          --no_https     : Use http for system tests'
  echo '          --use_ipv6     : Use use ipv6 for system tests'
  echo '          --help (-h)    : Display help'
}

sed -i 's/no_ssl =.*$/no_ssl = False/g' $BASEDIR/framework.py

source /root/.s3_ldap_cred_cache.conf

if [ $# -gt 0 ]
then
  while [ -n "$1" ]; do
    case "$1" in
      --no_https)
         sed -i 's/no_ssl =.*$/no_ssl = True/g' $BASEDIR/framework.py
         sed -i 's/use_https=.*$/use_https=false/g' $BASEDIR/jclient.properties
         sed -i 's/use_https=.*$/use_https=false/g' $BASEDIR/jcloud.properties
         shift ;;

      --use_ipv6)
         sed -i 's/use_ipv6 =.*$/use_ipv6 = True/g' $BASEDIR/framework.py
         shift ;;

      -h|--help) usage; exit 0 ;;

      *) echo 'Incorrect command. See runallsystest.sh --help for details.' ; exit 1 ;;
    esac
  done
else
  echo 'Executing ST`s over HTTPS'
fi

set -e
abort()
{
git checkout -- $BASEDIR/framework.py

    echo >&2 '
***************
*** ABORTED ***
***************
'
    echo "Error encountered. Exiting ST prematurely..." >&2
    trap : 0
    exit 1
}
trap 'abort' 0

sh ./prechecksystest.sh

# Update s3iamcli_test_config.yaml with the correct ldap info
sed -i "s/ldappasswd:.*/ldappasswd: \'$ldap_admin_pwd\'/g" ./s3iamcli_test_config.yaml
sed -i "s/password :.*/password : $ldap_root_pwd/g" test_data/ldap_config.yaml

#Using Python 3.6 version for Running System Tests
PythonV="python3.6"

#echo "`date -u`: Running backgrounddelete_spec.py"
#$PythonV backgrounddelete_spec.py

echo "`date -u`: Running auth_spec.py..."
$PythonV auth_spec.py

echo "`date -u`: Running awsiam_spec.py..."
$PythonV awsiam_spec.py

echo "`date -u`: Running auth_spec_param_validation.py..."
$PythonV auth_spec_param_validation.py

echo "`date -u`: Running auth_spec_signature_calculation.py..."
$PythonV auth_spec_signature_calculation.py

echo "`date -u`: Running s3cmd_spec.py..."
$PythonV s3cmd_spec.py

echo "`date -u`: Running jclient_spec.py..."
$PythonV jclient_spec.py

echo "`date -u`: Running jcloud_spec.py..."
$PythonV jcloud_spec.py

echo "`date -u`: Running rollback_spec.py..."
$PythonV rollback_spec.py

echo "`date -u`: Running negative_spec.py..."
$PythonV negative_spec.py

echo "`date -u`: Running shutdown_spec.py..."
$PythonV shutdown_spec.py

echo "`date -u`: Running awss3api_spec.py..."
$PythonV awss3api_spec.py

echo "`date -u`: Running aclvalidation_spec.py..."
$PythonV aclvalidation_spec.py

echo "`date -u`: Running policy_spec.py..."
$PythonV policy_spec.py

echo "`date -u`: Running authpassencryptcli_spec.py..."
$PythonV authpassencryptcli_spec.py

echo "`date -u`: Running auth_spec_negative_and_fi.py..."
$PythonV auth_spec_negative_and_fi.py

echo "`date -u`: Running s3_audit_log_schema.py..."
$PythonV s3_audit_log_schema.py

echo "$(date -u): Running md_integrity.py..."
# metadata integrity tests - regular PUT
md_di_data=/tmp/s3-data.bin
md_di_dowload=/tmp/s3-data-download.bin
md_di_parts=/tmp/s3-data-parts.json

dd if=/dev/urandom of=$md_di_data count=1 bs=1K
./md_integrity.py --body $md_di_data --download $md_di_dowload --parts $md_di_parts --test_plan ./regular_md_integrity.json

# metadata integrity tests - multipart
dd if=/dev/urandom of=$md_di_data count=1 bs=5M
./md_integrity.py --body $md_di_data --download $md_di_dowload --parts $md_di_parts --test_plan ./multipart_md_integrity.json

[ -f $md_di_data ] && rm -vf $md_di_data
[ -f $md_di_dowload ] && rm -vf $md_di_dowload
[ -f $md_di_parts ] && rm -vf $md_di_parts
# ==================================================

#echo "`date -u`: Running integrity.py..."
# Data Integrity tests:
# s3_config_port=$(grep -oE "S3_SERVER_BIND_PORT:\s*([0-9]?+)" /opt/seagate/cortx/s3/conf/s3config.yaml | tr -s ' ' | cut -d ' ' -f 2)
# enable DI FI
# curl -X PUT -H "x-seagate-faultinjection: enable,always,di_data_corrupted_on_write,0,0" "localhost:$s3_config_port"
# curl -X PUT -H "x-seagate-faultinjection: enable,always,di_data_corrupted_on_read,0,0" "localhost:$s3_config_port"
# run DI systest
# $USE_SUDO st/clitests/integrity.py --auto-test-all
# ======================================

git checkout -- $BASEDIR/framework.py
git checkout -- $BASEDIR/s3iamcli_test_config.yaml

echo >&2 '
**************************
*** ST Runs Successful ***
**************************
'
trap : 0
