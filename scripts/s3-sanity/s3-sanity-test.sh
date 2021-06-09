#!/bin/bash
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


#########################
# S3 sanity test script #
#########################

ldappasswd=''
s3iamclitestson=false
s3cmdtestson=false
anys3iamclitestsfailed=false
anys3cmdtestsfailed=false
anytestsfailed=false

s3cmdconfigfile="/tmp/.s3cfg"
test_file_input="/tmp/SanityObjectToDeleteAfterUse.input$$"
test_output_file="/tmp/SanityObjectToDeleteAfterUse.out$$"

set -e
set -p

USAGE="USAGE: bash $(basename "$0")
Run S3 sanity test.

PRE-REQUISITES: s3cmd and s3iamcli need to be present

where:
    -h     display this help and exit
    -c     clean s3 resources if present
    -e     specify the s3 server endpoint, pass 127.0.0.1 if running on s3 server node
           else provide data interface ip which needs to be in /etc/hosts
    -p     specify ldap password in text form, mandatory

Operations performed:
  * Create Account
  * Create User
  * Create Bucket
  * Put Object
  * Delete Object
  * Delete Bucket
  * Delete User
  * Delete Account
  * ldapsearch s3-background-delete-svc account"

# Print on stderr and exit with error code -1
die_with_error ()
{
  echo "$@" 1>&2
  exit -1
}

# Create s3iamcli config file if s3iamcli is enabled
# Create s3cmd config file if s3cmd is enabled
# Consider if end_point option is provided
update_config() {
  if [ "$s3iamclitestson" == false ];then
    echo "Both s3iamcli and s3cmd tests are disabled, nothing to config."
    return
  fi

  if [ "$s3cmdtestson" == true ];then
    echo "[default]" > $s3cmdconfigfile || die_with_error "unable to update $s3cmdconfigfile"
    echo "host_base = s3.seagate.com" >> $s3cmdconfigfile || die_with_error "unable to update $s3cmdconfigfile"
    echo "host_bucket = s3.seagate.com" >> $s3cmdconfigfile || die_with_error "unable to update $s3cmdconfigfile"
    echo "use_https = False" >> $s3cmdconfigfile || die_with_error "unable to update $s3cmdconfigfile"
  fi

  if [ ! -z "$end_point" ];then
    s3iamcli listaccounts --ldapuser sgiamadmin --ldappasswd "$ldappasswd" --no-ssl >/dev/null 2>&1 || echo "configured s3iamcli"
    echo "using s3endpoint $end_point"
    ls /root/.sgs3iamcli/config.yaml 2> /dev/null || die_with_error "S3iamcli configuration file is missing"

    if [ "$s3cmdtestson" == true ];then
      if [ ! -e $s3cmdconfigfile ];then
        die_with_error "S3cmd configuration file $s3cmdconfigfile is missing!"
      fi
      sed -i "s/host_base =.*/host_base = $end_point/g" $s3cmdconfigfile || die_with_error "unable to update $s3cmdconfigfile"
    fi

    sed -i "s/IAM:.*/IAM: http:\/\/$end_point:9080/g" /root/.sgs3iamcli/config.yaml || die_with_error "unable to update /root/.sgs3iamcli/config.yaml"
    sed -i "s/IAM_HTTPS:.*/IAM_HTTPS: https:\/\/$end_point:9443/g" /root/.sgs3iamcli/config.yaml || die_with_error "unable to update /root/.sgs3iamcli/config.yaml"
    sed -i "s/VERIFY_SSL_CERT:.*/VERIFY_SSL_CERT: false/g" /root/.sgs3iamcli/config.yaml || die_with_error "unable to update /root/.sgs3iamcli/config.yaml"
  fi
}

# Remove s3iamcli config file if s3iamcli is enabled
# Remove s3cmd config file if s3cmd is enabled
# Consider if end_point option is provided and undo the changes
restore_config() {
  if [ "$s3iamclitestson" == false ];then
    echo "Both s3iamcli and s3cmd tests are disabled, nothing to restore."
    return
  fi

  ls /root/.sgs3iamcli/config.yaml 2> /dev/null || die_with_error "S3iamcli configuration file is missing"
  rm -rf /root/.sgs3iamcli

  if [ "$s3cmdtestson" == true ];then
    ls $s3cmdconfigfile 2> /dev/null || die_with_error "S3cmd configuration file $s3cmdconfigfile is missing"
    ls $test_file_input 2> /dev/null || die_with_error "Input file $test_file_input is missing"
    ls $test_output_file 2> /dev/null || die_with_error "Output file  $test_output_file is missing"
    sed -i "s/host_base =.*/host_base = s3.seagate.com/g" $s3cmdconfigfile 
    rm -f $s3cmdconfigfile $test_file_input $test_output_file || die_with_error "rm -f failed in $s3cmdconfigfile $test_file_input $test_output_file"
  fi
}

# Call update_config()
# Remove s3iamcli user and accounts from ldap if s3iamcli is enabled
# Remove s3cmd data and bucket if s3cmd is enabled
# Call restore_config()
cleanup() {
  update_config
  if [ "$s3iamclitestson" == false ];then
    echo "Both s3iamcli and s3cmd tests are disabled, nothing to cleanup."
    return
  fi

  output=$(s3iamcli resetaccountaccesskey -n SanityAccountToDeleteAfterUse --ldapuser sgiamadmin --ldappasswd "$ldappasswd" --no-ssl) 2> /dev/null || echo ""
  if [[ $output == *"Name or service not known"*  || $output == *"Invalid argument"* ]];then
    restore_config
    die_with_error "Provide the Appropriate endpoint"
  elif [[ $output == *"NoSuchEntity"* ]];then
    echo "Sanity Account doesn't exist for cleanup"
    return
  else
    access_key=$(echo -e "$output" | tr ',' '\n' | grep "AccessKeyId" | awk '{print $3}')
    secret_key=$(echo -e "$output" | tr ',' '\n' | grep "SecretKey" | awk '{print $3}')

    if [ "$s3cmdtestson" == true ];then
      TEST_CMD="s3cmd --access_key=$access_key --secret_key=$secret_key -c $s3cmdconfigfile"
      $TEST_CMD del "s3://sanitybucket/SanityObjectToDeleteAfterUse" >/dev/null 2>&1 || echo ""
      $TEST_CMD rb "s3://sanitybucket" >/dev/null 2>&1 || echo ""
    fi

    s3iamcli deleteuser -n SanityUserToDeleteAfterUse --access_key "$access_key" --secret_key "$secret_key" --no-ssl 2> /dev/null || echo ""
    s3iamcli deleteaccount -n SanityAccountToDeleteAfterUse --access_key "$access_key" --secret_key "$secret_key" --no-ssl 2> /dev/null || echo ""
  fi

  restore_config
}

# Start of the script main
# Process command line args
while getopts ":p:e:c" o; do
    case "${o}" in
        e)
            end_point=${OPTARG}
            ;;
        c)
            externalcleanup=true
            ;;
        p)
            ldappasswd=${OPTARG}
            ;;
        *)
            echo "$USAGE"
            exit -1
            ;;
    esac
done
shift $((OPTIND-1))

trap "cleanup" ERR

if [ -z "$ldappasswd" ]
then
  die_with_error "Please provide -p <ldappasswd>"
fi

# Check if s3iamcli is installed
if command -v s3iamcli >/dev/null 2>&1;then
  echo "Check s3iamcli...OK, S3IAMCLI tests are enabled."
  s3iamclitestson=true

  # Check if s3cmd is installed
  if command -v s3cmd >/dev/null 2>&1;then
    echo "Check S3CMD...OK, S3CMD data path tests are enabled."
    s3cmdtestson=true
  else
    echo "S3CMD is not found, S3CMD (data path) tests are disabled."
  fi
else
  echo "S3IAMCLI is not found, both S3IAMCLI and S3CMD tests are disabled."
fi

cleanup

if [ "$externalcleanup" = true ];then
  # If "$externalcleanup" option provided, nothing to process further
  exit 0
fi

echo "*** S3 Sanity tests start ***"

echo "Replication test starts"
cmd_out="$(s3iamcli listaccounts --ldapuser sgiamadmin --ldappasswd "$ldappasswd" --no-ssl --showall)"
accounts_before="$(echo $cmd_out | grep AccountName | wc -l)"
END=10
for "((a=1;a<=$END;a++))"
do
	cmd_out=""
	cmd_out=$(s3iamcli listaccounts --ldapuser sgiamadmin --ldappasswd "$ldappasswd" --no-ssl --showall)
	accounts_after=$(echo $cmd_out | grep AccountName | wc -l)
	if "[ $accounts_before != $accounts_after ]"
	then
	   echo "S3 sanity test failed"
	   die_with_error "***** S3: SANITY Replication TESTS COMPLETED WITH FAILURE*****"
	fi
done
echo "Replication TESTS SUCCESSFULLY COMPLETED"

echo "ldapsearch test starts ***"
cmd_out=$(ldapsearch -b "o=s3-background-delete-svc,ou=accounts,dc=s3,dc=seagate,dc=com" -x -w $ldappasswd -D "cn=sgiamadmin,dc=seagate,dc=com" -H ldap://"$end_point") || echo ""
if [[ $cmd_out == *"No such object"* ]];then
  die_with_error "***** S3: SANITY ldapsearch TESTS COMPLETED WITH FAILURE, failed to find s3background delete account!*****"
fi
echo "ldapsearch TESTS SUCCESSFULLY COMPLETED"

if [ "$s3iamclitestson" == false ];then
  echo "S3IAMCLI tests are disabled"
  exit 0
fi
echo "S3IAMCLI tests start"
echo "Create Account"
output=$(s3iamcli createaccount -n SanityAccountToDeleteAfterUse  -e SanityAccountToDeleteAfterUse@sanitybucket.com --ldapuser sgiamadmin --ldappasswd "$ldappasswd" --no-ssl)
access_key=$(echo -e "$output" | tr ',' '\n' | grep "AccessKeyId" | awk '{print $3}')
secret_key=$(echo -e "$output" | tr ',' '\n' | grep "SecretKey" | awk '{print $3}')
s3iamcli CreateUser -n SanityUserToDeleteAfterUse --access_key "$access_key" --secret_key "$secret_key" --no-ssl 2> /dev/null
if [ $? -ne 0 ];then
  restore_config
  die_with_error "s3iamcli CreateUser failed"
fi

if [ "$s3cmdtestson" == true ];then
  echo "S3CMD tests start"
  TEST_CMD="s3cmd --access_key=$access_key --secret_key=$secret_key -v -c $s3cmdconfigfile"
  echo "Create bucket - 'sanitybucket': "
  $TEST_CMD mb "s3://sanitybucket"
  if [ $? -ne 0 ];then
    echo "Create bucket - 'sanitybucket':  failed"
    anys3cmdtestsfailed=true
  fi

  # create a test file
  dd if=/dev/urandom of=$test_file_input bs=5MB count=1
  if [ $? -ne 0 ];then
    echo "dd failed"
    anys3cmdtestsfailed=true
  fi
  content_md5_before=$(md5sum $test_file_input | cut -d ' ' -f 1)
  if [ $? -ne 0 ];then
    echo "md5sum failed"
    anys3cmdtestsfailed=true
  fi

  echo "Upload '$test_file_input' to 'sanitybucket': "
  $TEST_CMD put $test_file_input "s3://sanitybucket/SanityObjectToDeleteAfterUse"
  if [ $? -ne 0 ];then
    echo "Upload '$test_file_input' to 'sanitybucket': failed"
    anys3cmdtestsfailed=true
  fi

  echo "List uploaded SanityObjectToDeleteAfterUse in 'sanitybucket': "
  $TEST_CMD ls "s3://sanitybucket/SanityObjectToDeleteAfterUse"
  if [ $? -ne 0 ];then
    echo "List uploaded SanityObjectToDeleteAfterUse in 'sanitybucket': failed"
    anys3cmdtestsfailed=true
  fi

  echo "Download 'SanityObjectToDeleteAfterUse' from 'sanitybucket': "
  $TEST_CMD get "s3://sanitybucket/SanityObjectToDeleteAfterUse" $test_output_file
  if [ $? -ne 0 ];then
    echo "Download 'SanityObjectToDeleteAfterUse' from 'sanitybucket': failed"
    anys3cmdtestsfailed=true
  fi

  content_md5_after=$(md5sum $test_output_file | cut -d ' ' -f 1)
  echo "Data integrity check: "
  [[ $content_md5_before == $content_md5_after ]] && echo 'Passed.'
  echo "Delete 'SanityObjectToDeleteAfterUse' from 'sanitybucket': "
  $TEST_CMD del "s3://sanitybucket/SanityObjectToDeleteAfterUse"
  if [ $? -ne 0 ];then
    echo "s3://sanitybucket/SanityObjectToDeleteAfterUse failed"
    anys3cmdtestsfailed=true
  fi

  echo "Delete bucket - 'sanitybucket': "
  $TEST_CMD rb "s3://sanitybucket"
  if [ $? -ne 0 ];then
    echo "Delete bucket - 'sanitybucket': failed"
    anys3cmdtestsfailed=true
  fi

  if [ "$anys3cmdtestsfailed" == false ];then
    echo "S3: S3CMD SANITY DATA TESTS SUCCESSFULLY COMPLETED"
  else
    echo "S3: SANITY DATA TESTS COMPLETED WITH FAILURE"
    anytestsfailed=true
  fi
fi

echo "Delete User - 'SanityUserToDeleteAfterUse': "
s3iamcli deleteuser -n SanityUserToDeleteAfterUse --access_key "$access_key" --secret_key "$secret_key" --no-ssl 2> /dev/null
if [ $? -ne 0 ];then
  anys3iamclitestsfailed=true
  echo "s3iamcli deleteuser failed"
fi

echo "Delete Account - 'SanityAccountToDeleteAfterUse': "
s3iamcli deleteaccount -n SanityAccountToDeleteAfterUse --access_key "$access_key" --secret_key "$secret_key" --no-ssl
if [ $? -ne 0 ];then
  anys3iamclitestsfailed=true
  echo "s3iamcli deleteaccount failed"
fi

if [ "$anys3iamclitestsfailed" == false ];then
  echo "S3: SANITY IAMCLI TESTS SUCCESSFULLY COMPLETED"
else
  echo "S3: SANITY IAMCLI TESTS COMPLETED WITH FAILURE"
  anytestsfailed=true
fi

set +e

restore_config

if [ "$anytestsfailed" == false ];then
  echo "*** S3: SANITY ALL TESTS SUCCESSFULLY COMPLETED ***"
else
  die_with_error "*** S3: SANITY ALL TESTS COMPLETED WITH FAILURE ***"
fi
