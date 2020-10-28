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


set -e


#########################
# S3 sanity test script #
#########################

if command -v s3cmd >/dev/null 2>&1;then
    printf "\nCheck S3CMD...OK"
else
    printf "\nPlease install patched version built from <s3server src>/rpms/s3cmd/"
    exit 0
fi

# Check s3iamcli is installed
if command -v s3iamcli >/dev/null 2>&1; then
    printf "\nCheck s3iamcli...OK"
else
    printf "\ns3iamcli not installed"
    printf "\nPlease install s3iamcli using rpm built from <s3server repo>/rpms/s3iamcli/"
    exit 0
fi

test_file_input=/tmp/SanityObjectToDeleteAfterUse.input
test_output_file=/tmp/SanityObjectToDeleteAfterUse.out

ldappasswd=""
if rpm -q "salt"  > /dev/null;
then
    ldappasswd=$(salt-call pillar.get openldap:iam_admin:secret --output=newline_values_only)
    ldappasswd=$(salt-call lyveutil.decrypt openldap ${ldappasswd} --output=newline_values_only)
else
    # Dev environment. Read ldap admin password from "/root/.s3_ldap_cred_cache.conf"
    source /root/.s3_ldap_cred_cache.conf
fi

if [[ -z "$ldappasswd" ]]
then
    ldappasswd=$ldap_admin_pwd
fi

USAGE="USAGE: bash $(basename "$0")
Run S3 sanity test.
where:
    -h     display this help and exit
    -c     clean s3 resources if present
    -e     specify the s3 server endpoint, pass 127.0.0.1 if running on s3 server node
           else provide data interface ip which needs to be in /etc/hosts

Operations performed:
  * Create Account
  * Create User
  * Create Bucket
  * Put Object
  * Delete Object
  * Delete Bucket
  * Delete User
  * Delete Account"

update_config() {

   if [ ! -z $end_point ];then
     s3iamcli listaccounts --ldapuser sgiamadmin --ldappasswd "$ldappasswd" >/dev/null 2>&1 || echo "configured s3iamcli"
     echo "using s3endpoint $end_point"
     ls /root/.sgs3iamcli/config.yaml 2> /dev/null || { echo "S3iamcli configuration file is missing" && exit 1; }
     ls /root/.s3cfg 2> /dev/null || { echo "S3cmd configuration file is missing" && exit 1; }
     sed -i "s/IAM:.*/IAM: http:\/\/$end_point:9080/g" /root/.sgs3iamcli/config.yaml
     sed -i "s/IAM_HTTPS:.*/IAM_HTTPS: https:\/\/$end_point:9443/g" /root/.sgs3iamcli/config.yaml
     sed -i "s/VERIFY_SSL_CERT:.*/VERIFY_SSL_CERT: false/g" /root/.sgs3iamcli/config.yaml
     sed -i "s/host_base =.*/host_base = $end_point/g" /root/.s3cfg
   fi

}

restore_config() {
   ls /root/.sgs3iamcli/config.yaml 2> /dev/null || { echo "S3iamcli configuration file is missing" && exit 1; }
   ls /root/.s3cfg 2> /dev/null || { echo "S3cmd configuration file is missing" && exit 1; }
   rm -rf /root/.sgs3iamcli
   sed -i "s/host_base =.*/host_base = s3.seagate.com/g" /root/.s3cfg
}

cleanup() {
   update_config
   output=$(s3iamcli resetaccountaccesskey -n SanityAccountToDeleteAfterUse --ldapuser sgiamadmin --ldappasswd "$ldappasswd") 2> /dev/null || echo ""
   if [[ $output == *"Name or service not known"*  || $output == *"Invalid argument"* ]]
   then
       echo "Provide the Appropriate endpoint"
       restore_config
       exit 0
   elif [[ $output == *"NoSuchEntity"* ]]
   then
      echo "Sanity Account doesn't exist for cleanup"
      return;
   else
      access_key=$(echo -e "$output" | tr ',' '\n' | grep "AccessKeyId" | awk '{print $3}')
      secret_key=$(echo -e "$output" | tr ',' '\n' | grep "SecretKey" | awk '{print $3}')
      TEST_CMD="s3cmd --access_key=$access_key --secret_key=$secret_key"
      $TEST_CMD del "s3://sanitybucket/SanityObjectToDeleteAfterUse" >/dev/null 2>&1 || echo ""
      $TEST_CMD rb "s3://sanitybucket" >/dev/null 2>&1 || echo ""
      s3iamcli deleteuser -n SanityUserToDeleteAfterUse --access_key $access_key --secret_key $secret_key 2> /dev/null || echo ""
      s3iamcli deleteaccount -n SanityAccountToDeleteAfterUse --access_key $access_key --secret_key $secret_key 2> /dev/null || echo ""
      rm -f $test_file_input $test_output_file || echo ""
   fi
   restore_config
}

while getopts ":e:c" o; do
    case "${o}" in
        e)
            end_point=${OPTARG}
            ;;
        c)
            externalcleanup=true
            ;;
        *)
            echo "$USAGE"
            exit 0
            ;;
    esac
done
shift $((OPTIND-1))

trap "cleanup" ERR

if [ "$externalcleanup" = true ]
then
  cleanup
  exit 0
fi

cleanup
update_config

echo -e "\n\n*** S3 Sanity ***"
echo -e "\n\n**** Create Account *******"

output=$(s3iamcli createaccount -n SanityAccountToDeleteAfterUse  -e SanityAccountToDeleteAfterUse@sanitybucket.com --ldapuser sgiamadmin --ldappasswd "$ldappasswd")

access_key=$(echo -e "$output" | tr ',' '\n' | grep "AccessKeyId" | awk '{print $3}')
secret_key=$(echo -e "$output" | tr ',' '\n' | grep "SecretKey" | awk '{print $3}')

s3iamcli CreateUser -n SanityUserToDeleteAfterUse --access_key $access_key --secret_key $secret_key 2> /dev/null

TEST_CMD="s3cmd --access_key=$access_key --secret_key=$secret_key"

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

echo -e "\nDelete User - 'SanityUserToDeleteAfterUse': "
s3iamcli deleteuser -n SanityUserToDeleteAfterUse --access_key $access_key --secret_key $secret_key 2> /dev/null

echo -e "\nDelete Account - 'SanityAccountToDeleteAfterUse': "
s3iamcli deleteaccount -n SanityAccountToDeleteAfterUse --access_key $access_key --secret_key $secret_key

set +e

# delete test files file
rm -f $test_file_input $test_output_file

restore_config
echo -e "\n\n***** S3: SANITY TEST SUCCESSFULLY COMPLETED *****\n"
