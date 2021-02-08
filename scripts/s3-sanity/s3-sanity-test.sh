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

test_file_input=/tmp/SanityObjectToDeleteAfterUse.input
test_output_file=/tmp/SanityObjectToDeleteAfterUse.out
ldappasswd=''
data_path_test=false
s3cmdconfigfile="/root/.s3cfg"

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
    -d     run s3cmd data operations in the test
    -p     specify ldap password in text form, optional
    -f     specify s3cmd's config file, optional

Operations performed:
  * Create Account
  * Create User
  * Create Bucket
  * Put Object
  * Delete Object
  * Delete Bucket
  * Delete User
  * Delete Account"

die_with_error ()
{
  echo "$@" 1>&2
  exit -1
}

update_config() {

   if [ ! -z "$end_point" ];then
     s3iamcli listaccounts --ldapuser sgiamadmin --ldappasswd "$ldappasswd" --no-ssl >/dev/null 2>&1 || echo "configured s3iamcli"
     echo "using s3endpoint $end_point"
     ls /root/.sgs3iamcli/config.yaml 2> /dev/null || die_with_error "S3iamcli configuration file is missing"
     if [ "$data_path_test" == true ]
     then
       ls $s3cmdconfigfile 2> /dev/null || die_with_error "S3cmd configuration file $s3cmdconfigfile is missing"
       sed -i "s/host_base =.*/host_base = $end_point/g" $s3cmdconfigfile
     fi
     sed -i "s/IAM:.*/IAM: http:\/\/$end_point:9080/g" /root/.sgs3iamcli/config.yaml
     sed -i "s/IAM_HTTPS:.*/IAM_HTTPS: https:\/\/$end_point:9443/g" /root/.sgs3iamcli/config.yaml
     sed -i "s/VERIFY_SSL_CERT:.*/VERIFY_SSL_CERT: false/g" /root/.sgs3iamcli/config.yaml
   fi

}

restore_config() {
   ls /root/.sgs3iamcli/config.yaml 2> /dev/null || die_with_error "S3iamcli configuration file is missing"
     if [ "$data_path_test" == true ]
     then
       ls $s3cmdconfigfile 2> /dev/null || die_with_error "S3cmd configuration file $s3cmdconfigfile is missing"
       sed -i "s/host_base =.*/host_base = s3.seagate.com/g" $s3cmdconfigfile 
     fi
   rm -rf /root/.sgs3iamcli
}

cleanup() {
   update_config
   output=$(s3iamcli resetaccountaccesskey -n SanityAccountToDeleteAfterUse --ldapuser sgiamadmin --ldappasswd "$ldappasswd" --no-ssl) 2> /dev/null || echo ""
   if [[ $output == *"Name or service not known"*  || $output == *"Invalid argument"* ]]
   then
       echo "Provide the Appropriate endpoint"
       restore_config
       exit -1
   elif [[ $output == *"NoSuchEntity"* ]]
   then
      echo "Sanity Account doesn't exist for cleanup"
      return;
   else
      access_key=$(echo -e "$output" | tr ',' '\n' | grep "AccessKeyId" | awk '{print $3}')
      secret_key=$(echo -e "$output" | tr ',' '\n' | grep "SecretKey" | awk '{print $3}')
      TEST_CMD="s3cmd --access_key=$access_key --secret_key=$secret_key -c $s3cmdconfigfile"
     if [ "$data_path_test" == true ]
     then
       $TEST_CMD del "s3://sanitybucket/SanityObjectToDeleteAfterUse" >/dev/null 2>&1 || echo ""
       $TEST_CMD rb "s3://sanitybucket" >/dev/null 2>&1 || echo ""
      fi
      s3iamcli deleteuser -n SanityUserToDeleteAfterUse --access_key "$access_key" --secret_key "$secret_key" --no-ssl 2> /dev/null || echo ""
      s3iamcli deleteaccount -n SanityAccountToDeleteAfterUse --access_key "$access_key" --secret_key "$secret_key" --no-ssl 2> /dev/null || echo ""
      rm -f $test_file_input $test_output_file || echo ""
   fi
   restore_config
}

while getopts ":f:p:e:c:d" o; do
    case "${o}" in
        e)
            end_point=${OPTARG}
            ;;
        c)
            externalcleanup=true
            ;;
        d)
            data_path_test=true
            ;;
        p)
            ldappasswd=${OPTARG}
            ;;
        f)
            s3cmdconfigfile=${OPTARG}
            ;;
        *)
            echo "$USAGE"
            exit -1
            ;;
    esac
done
shift $((OPTIND-1))

trap "cleanup" ERR

if [ "$data_path_test" == true ]
then
  if command -v s3cmd >/dev/null 2>&1;then
      printf "\nCheck S3CMD...OK"
  else
      die_with_error "\ns3cmd not installed\nPlease install patched version built from <s3server src>/rpms/s3cmd/ or from rpm s3cmd"
  fi
  if [ ! -f "$s3cmdconfigfile" ]; then
      die_with_error "\ns3cmd config file $s3cmdconfigfile not found"
  fi
fi

# Check s3iamcli is installed
if command -v s3iamcli >/dev/null 2>&1; then
    printf "\nCheck s3iamcli...OK"
else
    die_with_error "\ns3iamcli not installed\nPlease install s3iamcli using rpm built from <s3server repo>/rpms/s3iamcli/ or from rpm cortx-s3iamcli"
fi

if [ -z "$ldappasswd" ]
then
  die_with_error "Please provide -p <ldappasswd>"
fi

systemctl restart s3authserver

if [ "$externalcleanup" = true ]
then
  cleanup
  exit 0
fi

cleanup
update_config

echo -e "\n\n*** S3 Sanity ***"
echo -e "\n\n**** Create Account *******"

output=$(s3iamcli createaccount -n SanityAccountToDeleteAfterUse  -e SanityAccountToDeleteAfterUse@sanitybucket.com --ldapuser sgiamadmin --ldappasswd "$ldappasswd" --no-ssl)

access_key=$(echo -e "$output" | tr ',' '\n' | grep "AccessKeyId" | awk '{print $3}')
secret_key=$(echo -e "$output" | tr ',' '\n' | grep "SecretKey" | awk '{print $3}')

s3iamcli CreateUser -n SanityUserToDeleteAfterUse --access_key "$access_key" --secret_key "$secret_key" --no-ssl 2> /dev/null

if [ "$data_path_test" == true ]
then
  TEST_CMD="s3cmd --access_key=$access_key --secret_key=$secret_key -v -c $s3cmdconfigfile"

  echo -e "\nCreate bucket - 'sanitybucket': "
  $TEST_CMD mb "s3://sanitybucket"

  # create a test file
  dd if=/dev/urandom of=$test_file_input bs=5MB count=1
  content_md5_before=$(md5sum $test_file_input | cut -d ' ' -f 1)

  echo -e "\nUpload '$test_file_input' to 'sanitybucket': "
  $TEST_CMD put $test_file_input "s3://sanitybucket/SanityObjectToDeleteAfterUse"

  echo -e "\nList uploaded SanityObjectToDeleteAfterUse in 'sanitybucket': "
  $TEST_CMD ls "s3://sanitybucket/SanityObjectToDeleteAfterUse"

  echo -e "\nDownload 'SanityObjectToDeleteAfterUse' from 'sanitybucket': "
  $TEST_CMD get "s3://sanitybucket/SanityObjectToDeleteAfterUse" $test_output_file
  content_md5_after=$(md5sum $test_output_file | cut -d ' ' -f 1)

  echo -en "\nData integrity check: "
  [[ $content_md5_before == $content_md5_after ]] && echo 'Passed.'

  echo -e "\nDelete 'SanityObjectToDeleteAfterUse' from 'sanitybucket': "
  $TEST_CMD del "s3://sanitybucket/SanityObjectToDeleteAfterUse"

  echo -e "\nDelete bucket - 'sanitybucket': "
  $TEST_CMD rb "s3://sanitybucket"

  echo -e "\n\n***** S3: SANITY DATA TESTS SUCCESSFULLY COMPLETED *****\n"
fi

echo -e "\nDelete User - 'SanityUserToDeleteAfterUse': "
s3iamcli deleteuser -n SanityUserToDeleteAfterUse --access_key "$access_key" --secret_key "$secret_key" --no-ssl 2> /dev/null

echo -e "\nDelete Account - 'SanityAccountToDeleteAfterUse': "
s3iamcli deleteaccount -n SanityAccountToDeleteAfterUse --access_key "$access_key" --secret_key "$secret_key" --no-ssl

set +e

# delete test files file
rm -f $test_file_input $test_output_file

restore_config
echo -e "\n\n***** S3: SANITY ALL TESTS SUCCESSFULLY COMPLETED *****\n"
