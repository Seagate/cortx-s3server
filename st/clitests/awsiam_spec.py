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

import os
import sys
import yaml
from framework import Config
from framework import S3PyCliTest
from awsiam import AwsIamTest
from awss3api import AwsTest
from s3client_config import S3ClientConfig
from s3cmd import S3cmdTest
from s3fi import S3fiTest
import shutil
from auth import AuthTest
from ldap_setup import LdapInfo

def prepare_test_data_for_user_policies(username):
    policy_arns = []
    non_attached_policy_arns = []
    for i in range(21):
        policy = os.path.join(os.path.dirname(__file__), 'policy_files', 'iam-policy.json')
        policy_testing = "file://" + os.path.abspath(policy)
        result = AwsIamTest('Create Policy').create_policy(f"iampolicyquota{i}",policy_testing) \
            .execute_test()
        policy_arn = (get_arn_from_policy_object(result.status.stdout))
        if i < 20:
            AwsIamTest('Attach User Policy').attach_user_policy(username, policy_arn) \
                .execute_test().command_is_successful()
            policy_arns.append(policy_arn)
        else:
            AwsIamTest('Attach User Policy').attach_user_policy(username, policy_arn) \
                .execute_test(negative_case=True).command_should_fail().command_error_should_have("InvalidRequest")
            non_attached_policy_arns.append(policy_arn)
    return policy_arns, non_attached_policy_arns

def get_keys_from_accesskey_object(raw_aws_cli_output):
    credentials = {}
    raw_lines = raw_aws_cli_output.split('\n')
    for _, item in enumerate(raw_lines):
        if (item.startswith("ACCESSKEY")):
            line = item.split('\t')
            credentials['AccessKeyId'] = line[1]
            credentials['SecretAccessKey'] = line[2]
        else:
            continue
    return credentials

def get_arn_from_policy_object(raw_aws_cli_output):
    raw_lines = raw_aws_cli_output.split('\n')
    for _, item in enumerate(raw_lines):
        if (item.startswith("POLICY")):
            line = item.split('\t')
            arn = line[1]
        else:
            continue
    return arn

def get_policy_list_count(raw_aws_cli_output, result_prefix):
    raw_lines = raw_aws_cli_output.split('\n')
    count = 0
    for _, item in enumerate(raw_lines):
        if (item.startswith(result_prefix)):
            count = count + 1
        else:
            continue
    return count

def user_tests():
    date_pattern = "[0-9|]+Z"
    #tests

    result = AwsIamTest('Create User').create_user("testUser").execute_test()
    result.command_response_should_have("testUser")

    result = AwsIamTest('CreateLoginProfile').create_login_profile("testUser","password").execute_test()
    login_profile_response_pattern = "LOGINPROFILE"+"[\s]*"+date_pattern+"[\s]*False[\s]*testUser"
    result.command_should_match_pattern(login_profile_response_pattern)
    result.command_response_should_have("testUser")

    result = AwsIamTest('GetLoginProfile Test').get_login_profile("testUser").execute_test()
    login_profile_response_pattern = "LOGINPROFILE"+"[\s]*"+date_pattern+"[\s]*False[\s]*testUser"
    result.command_should_match_pattern(login_profile_response_pattern)
    result.command_response_should_have("testUser")

    AwsIamTest('UpdateLoginProfile Test').update_login_profile("testUser").execute_test(negative_case=True).command_should_fail().command_error_should_have("InvalidRequest")

    AwsIamTest('UpdateLoginProfile with optional parameter- password').update_login_profile_with_optional_arguments\
        ("testUser","NewPassword",None,None).execute_test().command_is_successful()

    AwsIamTest('UpdateLoginProfile with optional parameter - password-reset-required').update_login_profile_with_optional_arguments\
        ("testUser",None,"password-reset-required",None).execute_test().command_is_successful()

    result = AwsIamTest('GetLoginProfile Test').get_login_profile("testUser").execute_test()
    login_profile_response_pattern = "LOGINPROFILE"+"[\s]*"+date_pattern+"[\s]*True[\s]*testUser"
    result.command_should_match_pattern(login_profile_response_pattern)
    result.command_response_should_have("True")

    AwsIamTest('Delete User').delete_user("testUser").execute_test().command_is_successful()

def iam_policy_crud_tests():
    #create-policy
    samplepolicy = os.path.join(os.path.dirname(__file__), 'policy_files', 'iam-policy.json')
    samplepolicy_testing = "file://" + os.path.abspath(samplepolicy)
    result = AwsIamTest('Create Policy').create_policy("iampolicy",samplepolicy_testing).execute_test()
    result.command_response_should_have("iampolicy")
    arn = (get_arn_from_policy_object(result.status.stdout))

    #create-policy
    samplepolicy = os.path.join(os.path.dirname(__file__), 'policy_files', 'iam-policy.json')
    samplepolicy_testing = "file://" + os.path.abspath(samplepolicy)
    result = AwsIamTest('Create Policy').create_policy("iampolicy2",samplepolicy_testing).execute_test()
    result.command_response_should_have("iampolicy2")
    arn2 = (get_arn_from_policy_object(result.status.stdout))


    #create-policy fails if policy with same name already exists
    AwsIamTest('Create Policy').create_policy("iampolicy",samplepolicy_testing).execute_test(negative_case=True)\
        .command_should_fail().command_error_should_have("EntityAlreadyExists")

    #get-policy
    AwsIamTest('Get Policy').get_policy(arn).execute_test().command_response_should_have("iampolicy")

    #get-policy-version
    AwsIamTest('Get Policy Version').get_policy_version(arn, "v1").execute_test() \
        .command_is_successful().command_response_should_have("POLICYVERSION")

    #get-policy-version with wrong version id
    AwsIamTest('Get Policy Version').get_policy_version(arn, "v4").execute_test(negative_case=True) \
        .command_should_fail().command_error_should_have("NoSuchEntity")

    #get-policy-version with wrong policy arn
    AwsIamTest('Get Policy Version').get_policy_version("arn:aws:iam::893266879798:policy/iampolicy67", "v1").execute_test(negative_case=True) \
        .command_should_fail().command_error_should_have("NoSuchEntity")

    #list-policies
    result = AwsIamTest('List Policies').list_policies().execute_test()
    total_policies = get_policy_list_count(result.status.stdout, "POLICIES")
    if(total_policies != 2):
        print('List Policies Test failed')
        quit()

    #delete-policy
    AwsIamTest('Delete Policy').delete_policy(arn).execute_test().command_is_successful()

    #get-policy on non-existing policy
    AwsIamTest('Get Policy').get_policy(arn).execute_test(negative_case=True).command_should_fail().command_error_should_have("NoSuchEntity")

    #delete-policy on non-existing policy
    AwsIamTest('Delete Policy').delete_policy(arn).execute_test(negative_case=True).command_should_fail().command_error_should_have("NoSuchEntity")

    test_msg = "Create account testAcc2"
    account_args = {'AccountName': 'testAcc2', 'Email': 'testAcc2@seagate.com', 'ldapuser': "sgiamadmin", 'ldappasswd': LdapInfo.get_ldap_admin_pwd()}
    account_response_pattern = "AccountId = [\w-]*, CanonicalId = [\w-]*, RootUserName = [\w+=,.@-]*, AccessKeyId = [\w-]*, SecretKey = [\w/+]*$"
    result = AuthTest(test_msg).create_account(**account_args).execute_test()
    result.command_should_match_pattern(account_response_pattern)
    account_response_elements = AuthTest.get_response_elements(result.status.stdout)
    testAcc2_access_key = account_response_elements['AccessKeyId']
    testAcc2_secret_key = account_response_elements['SecretKey']

    os.environ["AWS_ACCESS_KEY_ID"] = testAcc2_access_key
    os.environ["AWS_SECRET_ACCESS_KEY"] = testAcc2_secret_key

    #delete-policy cross account
    AwsIamTest('Delete Policy Cross Account Not Allowed').delete_policy(arn2).execute_test(negative_case=True).command_should_fail()

    del os.environ["AWS_ACCESS_KEY_ID"]
    del os.environ["AWS_SECRET_ACCESS_KEY"]

    # delete testAcc2 account
    test_msg = "Delete account testAcc2"
    account_args = {'AccountName': 'testAcc2', 'Email': 'testAcc2@seagate.com',  'force': True}
    S3ClientConfig.access_key_id = testAcc2_access_key
    S3ClientConfig.secret_key = testAcc2_secret_key
    AuthTest(test_msg).delete_account(**account_args).execute_test().command_is_successful()

    #delete-policy
    AwsIamTest('Delete Policy').delete_policy(arn2).execute_test().command_is_successful()

    #create-policy validation fails: version-missing
    samplepolicy = os.path.join(os.path.dirname(__file__), 'policy_files', 'iam-policy-version-missing.json')
    samplepolicy_testing = "file://" + os.path.abspath(samplepolicy)
    result = AwsIamTest('Policy Validation-Version Missing').create_policy("invalidPolicy1",samplepolicy_testing)\
    .execute_test(negative_case=True).command_should_fail().command_error_should_have("MalformedPolicy")

    #create-policy validation fails: statement-missing
    samplepolicy = os.path.join(os.path.dirname(__file__), 'policy_files', 'iam-policy-statement-missing.json')
    samplepolicy_testing = "file://" + os.path.abspath(samplepolicy)
    result = AwsIamTest('Policy Validation-Statement Missing').create_policy("invalidPolicy2",samplepolicy_testing)\
    .execute_test(negative_case=True).command_should_fail().command_error_should_have("MalformedPolicy")

    #create-policy validation fails: unknown-statement-element
    samplepolicy = os.path.join(os.path.dirname(__file__), 'policy_files', 'iam-policy-unknown-stmt-ele.json')
    samplepolicy_testing = "file://" + os.path.abspath(samplepolicy)
    result = AwsIamTest('Policy Validation-Unknown stmt ele').create_policy("invalidPolicy3",samplepolicy_testing)\
    .execute_test(negative_case=True).command_should_fail().command_error_should_have("MalformedPolicy")

    #create-policy validation fails: invalid-resource-arn
    samplepolicy = os.path.join(os.path.dirname(__file__), 'policy_files', 'iam-policy-invalid-resource-arn.json')
    samplepolicy_testing = "file://" + os.path.abspath(samplepolicy)
    result = AwsIamTest('Policy Validation-Invalid resource arn').create_policy("invalidPolicy4",samplepolicy_testing)\
    .execute_test(negative_case=True).command_should_fail().command_error_should_have("MalformedPolicy")

    #create-policy validation fails: invalid effect
    samplepolicy = os.path.join(os.path.dirname(__file__), 'policy_files', 'iam-policy-invalid-effect.json')
    samplepolicy_testing = "file://" + os.path.abspath(samplepolicy)
    result = AwsIamTest('Policy Validation-Invalid Effect').create_policy("invalidPolicy5",samplepolicy_testing)\
    .execute_test(negative_case=True).command_should_fail().command_error_should_have("MalformedPolicy")

    #create-policy validation fails: max-policy-size-exceed
    samplepolicy = os.path.join(os.path.dirname(__file__), 'policy_files', 'iam-policy-max-size-exceed.json')
    samplepolicy_testing = "file://" + os.path.abspath(samplepolicy)
    result = AwsIamTest('Policy Validation-Max size exceed').create_policy("invalidPolicy6",samplepolicy_testing)\
    .execute_test(negative_case=True).command_should_fail().command_error_should_have("LimitExceeded")

    #create-policy validation fails: duplicate-SIDs
    samplepolicy = os.path.join(os.path.dirname(__file__), 'policy_files', 'iam-policy-duplicate-sids.json')
    samplepolicy_testing = "file://" + os.path.abspath(samplepolicy)
    result = AwsIamTest('Policy Validation-Duplicate SIDs').create_policy("invalidPolicy7",samplepolicy_testing)\
    .execute_test(negative_case=True).command_should_fail().command_error_should_have("MalformedPolicy")

def user_policy_tests():
    #create-user
    result = AwsIamTest('Create User').create_user("testUser").execute_test()
    result.command_response_should_have("testUser")

    #create-user
    result = AwsIamTest('Create User').create_user("testUser2").execute_test()
    result.command_response_should_have("testUser2")

    #create-policy
    samplepolicy = os.path.join(os.path.dirname(__file__), 'policy_files', 'iam-policy.json')
    samplepolicy_testing = "file://" + os.path.abspath(samplepolicy)
    result = AwsIamTest('Create Policy').create_policy("iampolicy",samplepolicy_testing) \
        .execute_test()
    result.command_response_should_have("iampolicy")
    arn = (get_arn_from_policy_object(result.status.stdout))

    #create-policy
    samplepolicy = os.path.join(os.path.dirname(__file__), 'policy_files', 'iam-policy.json')
    samplepolicy_testing = "file://" + os.path.abspath(samplepolicy)
    result = AwsIamTest('Create Policy').create_policy_with_path("iampolicy2", \
        samplepolicy_testing, "/listpolicy/").execute_test()
    result.command_response_should_have("iampolicy2")
    arn2 = (get_arn_from_policy_object(result.status.stdout))

	#attach-user-policy
    AwsIamTest('Attach User Policy').attach_user_policy("testUser", arn) \
        .execute_test().command_is_successful()

    #list policies
    result = AwsIamTest('List Policies').list_policies().execute_test()
    total_policies = get_policy_list_count(result.status.stdout, "POLICIES")
    if(total_policies != 2):
        print('List Policies Test failed')
        quit()

    #list policies with only attached
    result = AwsIamTest('List Policies').list_policies(only_attached=True).execute_test()
    total_policies = get_policy_list_count(result.status.stdout, "POLICIES")
    if(total_policies != 1):
        print('List Policies with only attached Test failed')
        quit()

    #list policies with only attached and path prefix
    result = AwsIamTest('List Policies').list_policies(only_attached=True, path_prefix="/listpolicy/").execute_test()
    total_policies = get_policy_list_count(result.status.stdout, "POLICIES")
    if(total_policies != 0):
        print('List Policies with only attached and path prefix Test failed')
        quit()

    AwsIamTest('Attach User Policy').attach_user_policy("testUser", arn2) \
        .execute_test().command_is_successful()

    #list policies with only attached
    result = AwsIamTest('List Policies').list_policies(only_attached=True).execute_test()
    total_policies = get_policy_list_count(result.status.stdout, "POLICIES")
    if(total_policies != 2):
        print('List Policies with only attached Test failed')
        quit()

    #list policies with path prefix
    result = AwsIamTest('List Policies').list_policies(path_prefix="/listpolicy/").execute_test()
    total_policies = get_policy_list_count(result.status.stdout, "POLICIES")
    if(total_policies != 1):
        print('List Policies with path prefix Test failed')
        quit()

    #list policies with only attached and path prefix
    result = AwsIamTest('List Policies').list_policies(only_attached=True, path_prefix="/listpolicy/").execute_test()
    total_policies = get_policy_list_count(result.status.stdout, "POLICIES")
    if(total_policies != 1):
        print('List Policies with only attached and path prefix Test failed')
        quit()

    #attach already attached policy
    AwsIamTest('Attach User Policy').attach_user_policy("testUser", arn2) \
        .execute_test(negative_case=True).command_should_fail() \
            .command_error_should_have("EntityAlreadyExists")

    #attach policy to non-existing user
    AwsIamTest('Attach User Policy').attach_user_policy("jhjgj", arn2) \
        .execute_test(negative_case=True).command_should_fail() \
            .command_error_should_have("NoSuchEntity")

	#list user attached policies
    result = AwsIamTest('List User Attached Policies').list_attached_user_policies("testUser").execute_test()
    total_policies = get_policy_list_count(result.status.stdout, "ATTACHEDPOLICIES")
    if(total_policies != 2):
        print('List Attached User Policies Test failed')
        quit()
    result.command_response_should_have("iampolicy")

    #list user attached policies with path prefix filter
    result = AwsIamTest('List User Attached Policies').list_attached_user_policies("testUser", \
        path_prefix="/listpolicy/").execute_test()
    total_policies = get_policy_list_count(result.status.stdout, "ATTACHEDPOLICIES")
    if(total_policies != 1):
        print('List Attached User Policies with path prefix filter Test failed')
        quit()
    result.command_response_should_have("iampolicy2")

	#list user policies for user not having any policies attached
    result = AwsIamTest('List User Attached Policies').list_attached_user_policies("testUser2") \
        .execute_test()
    total_policies = get_policy_list_count(result.status.stdout, "ATTACHEDPOLICIES")
    if(total_policies != 0):
        print('List Attached User Policies for user not having any policies attached Test failed')
        quit()

    #list user policies for wrong username
    AwsIamTest('List User Attached Policies').list_attached_user_policies("asdhj") \
        .execute_test(negative_case=True).command_should_fail().command_error_should_have("NoSuchEntity")

    #delete policy attached to user
    AwsIamTest('Delete Policy').delete_policy(arn).execute_test(negative_case=True).command_should_fail() \
            .command_error_should_have("DeleteConflict")

	#detach-user-policy
    AwsIamTest('Detach User Policy').detach_user_policy("testUser", arn).execute_test() \
        .command_is_successful()
    AwsIamTest('Detach User Policy').detach_user_policy("testUser", arn2).execute_test() \
        .command_is_successful()

    #list policies with only attached after detaching policies
    result = AwsIamTest('List Policies').list_policies(only_attached=True).execute_test()
    total_policies = get_policy_list_count(result.status.stdout, "POLICIES")
    if(total_policies != 0):
        print('List Policies with only attached after detaching policies Test failed')
        quit()

    #detach-user-policy for non existing user
    result = AwsIamTest('Detach User Policy').detach_user_policy("hkhk", arn) \
        .execute_test(negative_case=True).command_should_fail().command_error_should_have("NoSuchEntity")

    #detach-user-policy with non existing policy
    result = AwsIamTest('Detach User Policy').detach_user_policy("testUser", "arn:aws:iam::103947660857:policy/oiuou") \
        .execute_test(negative_case=True).command_should_fail().command_error_should_have("NoSuchEntity")

    #detach-user-policy with non attached policy
    result = AwsIamTest('Detach User Policy').detach_user_policy("testUser", arn) \
        .execute_test(negative_case=True).command_should_fail().command_error_should_have("NoSuchEntity")

	#list user policies after detaching the policies
    result = AwsIamTest('List User Attached Policies').list_attached_user_policies("testUser").execute_test()
    total_policies = get_policy_list_count(result.status.stdout, "ATTACHEDPOLICIES")
    if(total_policies != 0):
        print('List Attached User Policies after detaching the policies Test failed')
        quit()

    #delete-policy
    result = AwsIamTest('Delete Policy').delete_policy(arn).execute_test().command_is_successful()
    result = AwsIamTest('Delete Policy').delete_policy(arn2).execute_test().command_is_successful()

    policy_arns = []
    non_attached_policy_arns = []
    policy_arns,non_attached_policy_arns = prepare_test_data_for_user_policies("testUser2")

    #list user attached policies
    result = AwsIamTest('List User Attached Policies').list_attached_user_policies("testUser2").execute_test()
    total_policies = get_policy_list_count(result.status.stdout, "ATTACHEDPOLICIES")
    if(total_policies != 20):
        print('List Attached User Policies Test failed')
        quit()

    for policy_arn in non_attached_policy_arns:
        result.command_response_should_not_have(policy_arn)
        AwsIamTest(f'Delete Policy {policy_arn}').delete_policy(policy_arn).execute_test().command_is_successful()

    for policy_arn in policy_arns:
        AwsIamTest(f'Detach User Policy {policy_arn}').detach_user_policy("testUser2", policy_arn).execute_test() \
                .command_is_successful()
        AwsIamTest(f'Delete Policy {policy_arn}').delete_policy(policy_arn).execute_test().command_is_successful()

    #delete-user
    result = AwsIamTest('Delete User').delete_user("testUser").execute_test().command_is_successful()
    result = AwsIamTest('Delete User').delete_user("testUser2").execute_test().command_is_successful()


def iam_policy_authorization_tests():
    #Only IAM Policy Present
    samplepolicy = os.path.join(os.path.dirname(__file__), 'policy_files', 'createpolicy-allow-iam-policy.json')
    samplepolicy_testing = "file://" + os.path.abspath(samplepolicy)
    result = AwsIamTest('Create Policy').create_policy("iampolicy1",samplepolicy_testing).execute_test().command_is_successful()
    arn = (get_arn_from_policy_object(result.status.stdout))
    AwsIamTest('Create User').create_user("testUser").execute_test().command_is_successful()
    result = AwsIamTest('Create AccessKey').create_access_key("testUser").execute_test().command_is_successful()
    keys = (get_keys_from_accesskey_object(result.status.stdout))
    AwsIamTest('Attach User policy').attach_user_policy("testUser",arn).execute_test().command_is_successful()
    os.environ["AWS_ACCESS_KEY_ID"] = keys['AccessKeyId']
    os.environ["AWS_SECRET_ACCESS_KEY"] = keys['SecretAccessKey']
    result = AwsIamTest('Create Policy').create_policy("iampolicy2",samplepolicy_testing).execute_test()
    result.command_response_should_have("iampolicy2")
    arn2 = (get_arn_from_policy_object(result.status.stdout))
    del os.environ["AWS_ACCESS_KEY_ID"]
    del os.environ["AWS_SECRET_ACCESS_KEY"]
    AwsIamTest('Detach User policy').detach_user_policy("testUser",arn).execute_test().command_is_successful()
    AwsIamTest('Delete Policy').delete_policy(arn).execute_test().command_is_successful()
    AwsIamTest('Delete Policy').delete_policy(arn2).execute_test().command_is_successful()

    #Allow in IAMPolicy and Allow in Bucket Policy
    AwsTest('Aws can create bucket').create_bucket("samplebucket").execute_test().command_is_successful()
    samplepolicy = os.path.join(os.path.dirname(__file__), 'policy_files', 'put-object-allow-bucket-policy.json')
    samplepolicy_testing = "file://" + os.path.abspath(samplepolicy)
    AwsTest("Aws can put policy on bucket").put_bucket_policy("samplebucket",samplepolicy_testing).execute_test().command_is_successful()
    samplepolicy = os.path.join(os.path.dirname(__file__), 'policy_files', 'put-object-allow-iam-policy.json')
    samplepolicy_testing = "file://" + os.path.abspath(samplepolicy)
    result = AwsIamTest('Create Policy').create_policy("iampolicy",samplepolicy_testing).execute_test().command_is_successful()
    arn = (get_arn_from_policy_object(result.status.stdout))
    AwsIamTest('Attach User policy').attach_user_policy("testUser",arn).execute_test().command_is_successful()
    os.environ["AWS_ACCESS_KEY_ID"] = keys['AccessKeyId']
    os.environ["AWS_SECRET_ACCESS_KEY"] = keys['SecretAccessKey']
    AwsTest('Put object to bucket').put_object("samplebucket", "testObject").execute_test().command_is_successful()
    del os.environ["AWS_ACCESS_KEY_ID"]
    del os.environ["AWS_SECRET_ACCESS_KEY"]
    AwsIamTest('Detach User policy').detach_user_policy("testUser",arn).execute_test().command_is_successful()
    AwsIamTest('Delete Policy').delete_policy(arn).execute_test().command_is_successful()
    AwsTest("Aws can delete policy on bucket").delete_bucket_policy("samplebucket").execute_test().command_is_successful()

    #Allow in IAM policy only
    samplepolicy = os.path.join(os.path.dirname(__file__), 'policy_files', 'get-object-allow-iam-policy.json')
    samplepolicy_testing = "file://" + os.path.abspath(samplepolicy)
    result = AwsIamTest('Create Policy').create_policy("testpolicy",samplepolicy_testing).execute_test().command_is_successful()
    arn = (get_arn_from_policy_object(result.status.stdout))
    AwsIamTest('Attach User policy').attach_user_policy("testUser",arn).execute_test().command_is_successful()
    os.environ["AWS_ACCESS_KEY_ID"] = keys['AccessKeyId']
    os.environ["AWS_SECRET_ACCESS_KEY"] = keys['SecretAccessKey']
    AwsTest('Aws can get object').get_object("samplebucket", "testObject").execute_test().command_is_successful()
    del os.environ["AWS_ACCESS_KEY_ID"]
    del os.environ["AWS_SECRET_ACCESS_KEY"]
    AwsIamTest('Detach User policy').detach_user_policy("testUser",arn).execute_test().command_is_successful()
    AwsIamTest('Delete Policy').delete_policy(arn).execute_test().command_is_successful()
    AwsTest('Aws can delete object').delete_object("samplebucket","testObject").execute_test().command_is_successful()

    #Deny in IAM Policy and Allow in Bucket Policy
    samplepolicy = os.path.join(os.path.dirname(__file__), 'policy_files', 'put-object-allow-bucket-policy.json')
    samplepolicy_testing = "file://" + os.path.abspath(samplepolicy)
    AwsTest("Aws can put policy on bucket").put_bucket_policy("samplebucket",samplepolicy_testing).execute_test().command_is_successful()
    samplepolicy = os.path.join(os.path.dirname(__file__), 'policy_files', 'put-object-deny-iam-policy.json')
    samplepolicy_testing = "file://" + os.path.abspath(samplepolicy)
    result = AwsIamTest('Create Policy').create_policy("iampolicy",samplepolicy_testing).execute_test().command_is_successful()
    arn = (get_arn_from_policy_object(result.status.stdout))
    AwsIamTest('Attach User policy').attach_user_policy("testUser",arn).execute_test().command_is_successful()
    os.environ["AWS_ACCESS_KEY_ID"] = keys['AccessKeyId']
    os.environ["AWS_SECRET_ACCESS_KEY"] = keys['SecretAccessKey']
    AwsTest('Put object to bucket').put_object("samplebucket", "testObject").execute_test(negative_case=True)\
    .command_should_fail().command_error_should_have("AccessDenied")
    del os.environ["AWS_ACCESS_KEY_ID"]
    del os.environ["AWS_SECRET_ACCESS_KEY"]
    AwsIamTest('Detach User policy').detach_user_policy("testUser",arn).execute_test().command_is_successful()
    AwsIamTest('Delete Policy').delete_policy(arn).execute_test().command_is_successful()
    AwsTest("Aws can delete policy on bucket").delete_bucket_policy("samplebucket").execute_test().command_is_successful()

    #Allow in IAM Policy and Deny in Bucket Policy
    samplepolicy = os.path.join(os.path.dirname(__file__), 'policy_files', 'put-object-deny-bucket-policy.json')
    samplepolicy_testing = "file://" + os.path.abspath(samplepolicy)
    AwsTest("Aws can put policy on bucket").put_bucket_policy("samplebucket",samplepolicy_testing).execute_test().command_is_successful()
    samplepolicy = os.path.join(os.path.dirname(__file__), 'policy_files', 'put-object-allow-iam-policy.json')
    samplepolicy_testing = "file://" + os.path.abspath(samplepolicy)
    result = AwsIamTest('Create Policy').create_policy("iampolicy",samplepolicy_testing).execute_test().command_is_successful()
    arn = (get_arn_from_policy_object(result.status.stdout))
    AwsIamTest('Attach User policy').attach_user_policy("testUser",arn).execute_test().command_is_successful()
    os.environ["AWS_ACCESS_KEY_ID"] = keys['AccessKeyId']
    os.environ["AWS_SECRET_ACCESS_KEY"] = keys['SecretAccessKey']
    AwsTest('Put object to bucket').put_object("samplebucket", "testObject").execute_test(negative_case=True)\
    .command_should_fail().command_error_should_have("AccessDenied")
    del os.environ["AWS_ACCESS_KEY_ID"]
    del os.environ["AWS_SECRET_ACCESS_KEY"]
    AwsIamTest('Detach User policy').detach_user_policy("testUser",arn).execute_test().command_is_successful()
    AwsIamTest('Delete Policy').delete_policy(arn).execute_test().command_is_successful()
    AwsTest("Aws can delete policy on bucket").delete_bucket_policy("samplebucket").execute_test().command_is_successful()

    #IAM operation and No IAM policy
    os.environ["AWS_ACCESS_KEY_ID"] = keys['AccessKeyId']
    os.environ["AWS_SECRET_ACCESS_KEY"] = keys['SecretAccessKey']
    AwsIamTest('List Access Keys').list_access_keys("testUser").execute_test().command_is_successful()
    del os.environ["AWS_ACCESS_KEY_ID"]
    del os.environ["AWS_SECRET_ACCESS_KEY"]

    #deny in iam policy but should be allowed for root account
    samplepolicy = os.path.join(os.path.dirname(__file__), 'policy_files', 'get-policy-deny-iam-policy.json')
    samplepolicy_testing = "file://" + os.path.abspath(samplepolicy)
    result = AwsIamTest('Create Policy').create_policy("iampolicy",samplepolicy_testing).execute_test().command_is_successful()
    arn = (get_arn_from_policy_object(result.status.stdout))
    AwsIamTest('Attach User policy').attach_user_policy("testUser",arn).execute_test().command_is_successful()
    os.environ["AWS_ACCESS_KEY_ID"] = keys['AccessKeyId']
    os.environ["AWS_SECRET_ACCESS_KEY"] = keys['SecretAccessKey']
    AwsIamTest('Get Policy').get_policy(arn).execute_test(negative_case=True).command_should_fail().command_error_should_have("AccessDenied")
    del os.environ["AWS_ACCESS_KEY_ID"]
    del os.environ["AWS_SECRET_ACCESS_KEY"]
    AwsIamTest('Get Policy').get_policy(arn).execute_test().command_is_successful()
    AwsIamTest('Detach User policy').detach_user_policy("testUser",arn).execute_test().command_is_successful()
    AwsIamTest('Delete Policy').delete_policy(arn).execute_test().command_is_successful()


    AwsTest('Aws can delete bucket').delete_bucket("samplebucket").execute_test().command_is_successful()
    AwsIamTest('Delete AccessKey').delete_access_key(keys['AccessKeyId']).execute_test().command_is_successful()
    AwsIamTest('Delete User').delete_user("testUser").execute_test().command_is_successful()

if __name__ == '__main__':

    user_tests()
    iam_policy_crud_tests()
    user_policy_tests()
    iam_policy_authorization_tests()
