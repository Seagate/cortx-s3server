import os
import yaml
import json
from framework import Config
from framework import S3PyCliTest
from awss3api import AwsTest
from s3cmd import S3cmdTest
from s3client_config import S3ClientConfig
from aclvalidation import AclTest
from auth import AuthTest

# Helps debugging
Config.log_enabled = True
# Config.dummy_run = True
# Config.client_execution_timeout = 300 * 1000
# Config.request_timeout = 300 * 1000
# Config.socket_timeout = 300 * 1000

policy_relative = os.path.join(os.path.dirname(__file__), 'policy_files', 'policy.txt')
policy = "file://" + os.path.abspath(policy_relative)

policy_put_bucket_relative = os.path.join(os.path.dirname(__file__), 'policy_files', 'policy_put_bucket.txt')
policy_put_bucket = "file://" + os.path.abspath(policy_put_bucket_relative)

policy_condition_StringEquals_success_relative = os.path.join(os.path.dirname(__file__), 'policy_files', 'policy_condition_StringEquals_success.json')
policy_condition_StringEquals_success = "file://" + os.path.abspath(policy_condition_StringEquals_success_relative)

policy_condition_ArnLike_success_relative = os.path.join(os.path.dirname(__file__), 'policy_files', 'policy_condition_ArnLike_success.json')
policy_condition_ArnLike_success = "file://" + os.path.abspath(policy_condition_ArnLike_success_relative)

policy_condition_StringLike_success_relative = os.path.join(os.path.dirname(__file__), 'policy_files', 'policy_condition_StringLike_success.json')
policy_condition_StringLike_success = "file://" + os.path.abspath(policy_condition_StringLike_success_relative)

policy_condition_StringEqualsIfExists_success_relative = os.path.join(os.path.dirname(__file__), 'policy_files', 'policy_condition_StringEqualsIfExists_success.json')
policy_condition_StringEqualsIfExists_success = "file://" + os.path.abspath(policy_condition_StringEqualsIfExists_success_relative)

policy_condition_Bool_success_relative = os.path.join(os.path.dirname(__file__), 'policy_files', 'policy_condition_Bool_success.json')
policy_condition_Bool_success = "file://" + os.path.abspath(policy_condition_Bool_success_relative)

policy_condition_Bool_RandomKeyValue_success_relative = os.path.join(os.path.dirname(__file__), 'policy_files', 'policy_condition_Bool_RandomKeyValue_success.json')
policy_condition_Bool_RandomKeyValue_success = "file://" + os.path.abspath(policy_condition_Bool_RandomKeyValue_success_relative)

policy_condition_Bool_invalidKey_fail_relative = os.path.join(os.path.dirname(__file__), 'policy_files', 'policy_condition_Bool_invalidKey_fail.json')
policy_condition_Bool_invalidKey_fail = "file://" + os.path.abspath(policy_condition_Bool_invalidKey_fail_relative)

policy_condition_StringEquals_invalidKey_fail_relative = os.path.join(os.path.dirname(__file__), 'policy_files', 'policy_condition_StringEquals_invalidKey_fail.json')
policy_condition_StringEquals_invalidKey_fail = "file://" + os.path.abspath(policy_condition_StringEquals_invalidKey_fail_relative)

policy_condition_NumericLessThanEquals_success_relative = os.path.join(os.path.dirname(__file__), 'policy_files', 'policy_condition_NumericLessThanEquals_success.json')
policy_condition_NumericLessThanEquals_success = "file://" + os.path.abspath(policy_condition_NumericLessThanEquals_success_relative)

policy_condition_DateLessThan_success_relative = os.path.join(os.path.dirname(__file__), 'policy_files', 'policy_condition_DateLessThan_success.json')
policy_condition_DateLessThan_success = "file://" + os.path.abspath(policy_condition_DateLessThan_success_relative)

policy_condition_BinaryEquals_success_relative = os.path.join(os.path.dirname(__file__), 'policy_files', 'policy_condition_BinaryEquals_success.json')
policy_condition_BinaryEquals_success = "file://" + os.path.abspath(policy_condition_BinaryEquals_success_relative)

policy_condition_BinaryEquals_invalidValue_fail_relative = os.path.join(os.path.dirname(__file__), 'policy_files', 'policy_condition_BinaryEquals_invalidValue_fail.json')
policy_condition_BinaryEquals_invalidValue_fail = "file://" + os.path.abspath(policy_condition_BinaryEquals_invalidValue_fail_relative)

policy_condition_StringLike_invalidKey_fail_relative = os.path.join(os.path.dirname(__file__), 'policy_files', 'policy_condition_StringLike_invalidKey_fail.json')
policy_condition_StringLike_invalidKey_fail = "file://" + os.path.abspath(policy_condition_StringLike_invalidKey_fail_relative)

policy_condition_StringLikeIfExists_success_relative = os.path.join(os.path.dirname(__file__), 'policy_files', 'policy_condition_StringLikeIfExists_success.json')
policy_condition_StringLikeIfExists_success = "file://" + os.path.abspath(policy_condition_StringLikeIfExists_success_relative)

policy_condition_combination_fail_relative = os.path.join(os.path.dirname(__file__), 'policy_files', 'policy_condition_combination_fail.json')
policy_condition_combination_fail = "file://" + os.path.abspath(policy_condition_combination_fail_relative)

policy_for_iam_user_relative = os.path.join(os.path.dirname(__file__), 'policy_files', 'newaccount_policy.txt')
policy_for_iam_user = "file://" + os.path.abspath(policy_for_iam_user_relative)

deny_policy_for_iam_user_relative = os.path.join(os.path.dirname(__file__), 'policy_files', 'deny_newaccount_policy.txt')
deny_policy_for_iam_user = "file://" + os.path.abspath(deny_policy_for_iam_user_relative)

policy_access_only_to_owner_relative = os.path.join(os.path.dirname(__file__), 'policy_files', 'deny_newaccount_policy.txt')
policy_access_only_to_owner = "file://" + os.path.abspath(policy_access_only_to_owner_relative)

policy_authorization_relative = os.path.join(os.path.dirname(__file__), 'policy_files', 'deny_policy_allow_acl.txt')
policy_authorization = "file://" + os.path.abspath(policy_authorization_relative)



AwsTest('Aws can create bucket').create_bucket("seagate").execute_test().command_is_successful()

AwsTest("Aws can get policy on bucket").get_bucket_policy("seagate").execute_test(negative_case=True).command_should_fail().command_error_should_have("NoSuchBucketPolicy")

AwsTest("Aws can delete policy on bucket").delete_bucket_policy("seagate").execute_test().command_is_successful()

AwsTest("Aws can put policy on bucket").put_bucket_policy("seagate", policy).execute_test().command_is_successful()

AwsTest("Aws can get policy on bucket").get_bucket_policy("seagate").execute_test().command_is_successful().command_response_should_have("Resource")

AwsTest("Aws can delete policy on bucket").delete_bucket_policy("seagate").execute_test().command_is_successful()

AwsTest("Aws can get policy on bucket").get_bucket_policy("seagate").execute_test(negative_case=True).command_should_fail().command_error_should_have("NoSuchBucketPolicy")

AwsTest("Aws can put policy on bucket").put_bucket_policy("seagate", policy_put_bucket).execute_test().command_is_successful()

AwsTest("Aws can get policy on bucket").get_bucket_policy("seagate").execute_test().command_is_successful().command_response_should_have("Resource")

AwsTest("Aws can delete policy on bucket").delete_bucket_policy("seagate").execute_test().command_is_successful()

AwsTest("Aws can get policy on bucket").get_bucket_policy("seagate").execute_test(negative_case=True).command_should_fail().command_error_should_have("NoSuchBucketPolicy")

#put-bucket-policy - Effect field absent
policy_put_bucket_relative = os.path.join(os.path.dirname(__file__), 'policy_files', 'policy_put_bucket_effect_missing.txt')
policy_put_bucket = "file://" + os.path.abspath(policy_put_bucket_relative)
AwsTest("Put Bucket Policy with Effect field missing").put_bucket_policy("seagate", policy_put_bucket).execute_test(negative_case=True).command_should_fail().command_error_should_have("MalformedPolicy")


#put-bucket-policy - Resource-Action field unrelated
policy_put_bucket_relative = os.path.join(os.path.dirname(__file__), 'policy_files', 'policy_put_bucket_resource_action_unrelated.txt')
policy_put_bucket = "file://" + os.path.abspath(policy_put_bucket_relative)
AwsTest("Put Bucket Policy with unrelated Resource and Action").put_bucket_policy("seagate", policy_put_bucket).execute_test(negative_case=True).command_should_fail().command_error_should_have("MalformedPolicy")


#put-bucket-policy - Action field accepts wild-card characters
policy_put_bucket_relative = os.path.join(os.path.dirname(__file__), 'policy_files', 'policy_put_bucket_action_with_wildcards.txt')
policy_put_bucket = "file://" + os.path.abspath(policy_put_bucket_relative)
AwsTest("Put Bucket Policy with WildCard chars in Action").put_bucket_policy("seagate", policy_put_bucket).execute_test().command_is_successful()


#put-bucket-policy - Invalid field in policy
policy_put_bucket_relative = os.path.join(os.path.dirname(__file__), 'policy_files', 'policy_put_bucket_invalid_field_present.txt')
policy_put_bucket = "file://" + os.path.abspath(policy_put_bucket_relative)
AwsTest("Put Bucket Policy with invalid field in policy json").put_bucket_policy("seagate", policy_put_bucket).execute_test(negative_case=True).command_should_fail().command_error_should_have("MalformedPolicy")


#put-bucket-policy - Fields not in order
policy_put_bucket_relative = os.path.join(os.path.dirname(__file__), 'policy_files', 'policy_put_bucket_improper_field_order.txt')
policy_put_bucket = "file://" + os.path.abspath(policy_put_bucket_relative)
AwsTest("Put Bucket Policy with improper order of fields in policy json").put_bucket_policy("seagate", policy_put_bucket).execute_test(negative_case=True).command_should_fail().command_error_should_have("MalformedPolicy")


#put-bucket-policy - Multiple resources
policy_put_bucket_relative = os.path.join(os.path.dirname(__file__), 'policy_files', 'policy_put_bucket_with_multiple_resources.txt')
policy_put_bucket = "file://" + os.path.abspath(policy_put_bucket_relative)
AwsTest("Put Bucket Policy with multiple resources in policy json").put_bucket_policy("seagate", policy_put_bucket).execute_test().command_is_successful()

#put-bucket-policy - Invalid Action
policy_put_bucket_relative = os.path.join(os.path.dirname(__file__), 'policy_files', 'policy_put_bucket_with_invalid_action.txt')
policy_put_bucket = "file://" + os.path.abspath(policy_put_bucket_relative)
AwsTest("Put Bucket Policy with invalid Action in policy json").put_bucket_policy("seagate", policy_put_bucket).execute_test(negative_case=True).command_should_fail().command_error_should_have("MalformedPolicy")

#put-bucket-policy - Invalid Principal
policy_put_bucket_relative = os.path.join(os.path.dirname(__file__), 'policy_files', 'policy_put_bucket_invalid_principal.txt')
policy_put_bucket = "file://" + os.path.abspath(policy_put_bucket_relative)
AwsTest("Put Bucket Policy with invalid Principal ID in policy json").put_bucket_policy("seagate", policy_put_bucket).execute_test(negative_case=True).command_should_fail().command_error_should_have("MalformedPolicy")


#put-bucket-policy - Valid principal
policy_put_bucket_relative = os.path.join(os.path.dirname(__file__), 'policy_files', 'policy_put_bucket_with_valid_principal.txt')
policy_put_bucket = "file://" + os.path.abspath(policy_put_bucket_relative)
AwsTest("Put Bucket Policy with valid Principal in policy json").put_bucket_policy("seagate", policy_put_bucket).execute_test().command_is_successful()




############### Authorize Policy #######################
print("Authorizing policy tests start....")

def load_test_config():
    conf_file = os.path.join(os.path.dirname(__file__),'s3iamcli_test_config.yaml')
    with open(conf_file, 'r') as f:
            config = yaml.safe_load(f)
            S3ClientConfig.ldapuser = config['ldapuser']
            S3ClientConfig.ldappasswd = config['ldappasswd']


load_test_config()
#put-bucket-policy - Valid principal
policy_put_bucket_relative = os.path.join(os.path.dirname(__file__), 'policy_files', 'get_bucket_policy_restricted_cross_account.txt')
policy_put_bucket = "file://" + os.path.abspath(policy_put_bucket_relative)
AwsTest("Put Bucket Policy with Allow access for all").put_bucket_policy("seagate", policy_put_bucket).execute_test().command_is_successful()

# Create secondary account
test_msg = "Create account newaccount"
account_args = {'AccountName': 'newaccount', 'Email': 'newaccount@seagate.com', 'ldapuser': S3ClientConfig.ldapuser, 'ldappasswd': S3ClientConfig.ldappasswd}
account_response_pattern = "AccountId = [\w-]*, CanonicalId = [\w-]*, RootUserName = [\w+=,.@-]*, AccessKeyId = [\w-]*, SecretKey = [\w/+]*$"
result = AuthTest(test_msg).create_account(**account_args).execute_test()
result.command_should_match_pattern(account_response_pattern)
account_response_elements = AuthTest.get_response_elements(result.status.stdout)
secondary_access_key = account_response_elements['AccessKeyId']
secondary_secret_key = account_response_elements['SecretKey']
S3ClientConfig.access_key_id = secondary_access_key
S3ClientConfig.secret_key = secondary_secret_key
print(secondary_access_key)
print(secondary_secret_key)
os.environ["AWS_ACCESS_KEY_ID"] = secondary_access_key
os.environ["AWS_SECRET_ACCESS_KEY"] = secondary_secret_key

AwsTest("Cross account can not perform GetBucketPolicy").get_bucket_policy("seagate").execute_test(negative_case=True).command_should_fail().command_error_should_have("MethodNotAllowed")

# Test access for IAM user under bucket owner account
# IAM user can peform Get,PUT and DELETE bucket policy operations if granted access
# Bucket owenr can perform Get,PUT and DELETE bucket policy operations even if denied access in policy
AwsTest('Aws can create bucket').create_bucket("newaccountbucket").execute_test().command_is_successful()
AwsTest("Aws can put policy on bucket").put_bucket_policy("newaccountbucket",policy_for_iam_user).execute_test().command_is_successful()
AwsTest("Aws can get policy on bucket").get_bucket_policy("newaccountbucket").execute_test().command_is_successful()
account_args['UserName'] = "u1"
test_msg = "Create User u1"
user1_response_pattern = "UserId = [\w-]*, ARN = [\S]*, Path = /$"
result = AuthTest(test_msg).create_user(**account_args).execute_test()
result.command_should_match_pattern(user1_response_pattern)
user_access_key_args = {}
accesskey_response_pattern = "AccessKeyId = [\w-]*, SecretAccessKey = [\w/+]*, Status = [\w]*$"
result = AuthTest(test_msg).create_access_key(**account_args).execute_test()
result.command_should_match_pattern(accesskey_response_pattern)
accesskey_response_elements = AuthTest.get_response_elements(result.status.stdout)
user_access_key_args['AccessKeyId'] = accesskey_response_elements['AccessKeyId']
user_access_key_args['SecretAccessKey'] = accesskey_response_elements['SecretAccessKey']
user_access_key_args['UserName'] = "u1"
os.environ["AWS_ACCESS_KEY_ID"] = accesskey_response_elements['AccessKeyId']
os.environ["AWS_SECRET_ACCESS_KEY"] = accesskey_response_elements['SecretAccessKey']

AwsTest("IAM user can get policy on bucket").get_bucket_policy("newaccountbucket").execute_test().command_is_successful()
AwsTest("IAM user can delete policy on bucket").delete_bucket_policy("newaccountbucket").execute_test().command_is_successful()
#Setting Deny in Policy for bucket Owner
AwsTest("Aws can put policy on bucket").put_bucket_policy("newaccountbucket",deny_policy_for_iam_user).execute_test().command_is_successful()
os.environ["AWS_ACCESS_KEY_ID"] = secondary_access_key
os.environ["AWS_SECRET_ACCESS_KEY"] = secondary_secret_key
#Switching back to newaccount and checkig if owner can perform GET,PUT and DELETE policy though Deny in Policy
AwsTest("IAM user can get policy on bucket").get_bucket_policy("newaccountbucket").execute_test().command_is_successful()
AwsTest("IAM user can delete policy on bucket").delete_bucket_policy("newaccountbucket").execute_test().command_is_successful()
AwsTest("Aws can put policy on bucket").put_bucket_policy("newaccountbucket",policy_access_only_to_owner).execute_test().command_is_successful()
#As per permissions provided in above new policy , iam users will not be able to execute any operations
os.environ["AWS_ACCESS_KEY_ID"] = accesskey_response_elements['AccessKeyId']
os.environ["AWS_SECRET_ACCESS_KEY"] = accesskey_response_elements['SecretAccessKey']

AwsTest("IAM users restricted get policy on bucket").get_bucket_policy("newaccountbucket").execute_test(negative_case=True).command_should_fail()
AwsTest("IAM user restricted  delete policy on bucket").delete_bucket_policy("newaccountbucket").execute_test(negative_case=True).command_should_fail()
AwsTest("Aws can put policy on bucket").put_bucket_policy("newaccountbucket",policy_access_only_to_owner).execute_test(negative_case=True).command_should_fail()

os.environ["AWS_ACCESS_KEY_ID"] = secondary_access_key
os.environ["AWS_SECRET_ACCESS_KEY"] = secondary_secret_key

AwsTest('Aws can delete bucket').delete_bucket("newaccountbucket").execute_test().command_is_successful()
test_msg = 'Delete access key'
result = AuthTest(test_msg).delete_access_key(**user_access_key_args).execute_test()
result.command_response_should_have("Access key deleted.")
result = AuthTest(test_msg).delete_user(**account_args).execute_test()
result.command_response_should_have("User deleted.")


############# Deny in Policy but Allow in ACL ###############

AwsTest('Aws can create bucket').create_bucket("auth-bucket").execute_test().command_is_successful()

AwsTest('Aws can upload file').put_object_with_permission_headers("auth-bucket", "samplefile", "grant-read-acp" , "id=C12345" ).execute_test().command_is_successful()

AwsTest("Aws can put policy on bucket").put_bucket_policy("auth-bucket",policy_authorization).execute_test().command_is_successful()


del os.environ["AWS_ACCESS_KEY_ID"]
del os.environ["AWS_SECRET_ACCESS_KEY"]

AwsTest('Aws can get object acl').get_object_acl("auth-bucket", "samplefile").execute_test(negative_case=True).command_should_fail().command_error_should_have("AccessDenied")
################## Allow in Policy and No permission in ACL #################
os.environ["AWS_ACCESS_KEY_ID"] = secondary_access_key
os.environ["AWS_SECRET_ACCESS_KEY"] = secondary_secret_key

policy_authorization_relative = os.path.join(os.path.dirname(__file__), 'policy_files', 'allow_policy_nopermission_acl.txt')
policy_authorization = "file://" + os.path.abspath(policy_authorization_relative)

AwsTest("Aws can put policy on bucket").put_bucket_policy("auth-bucket",policy_authorization).execute_test().command_is_successful()

del os.environ["AWS_ACCESS_KEY_ID"]
del os.environ["AWS_SECRET_ACCESS_KEY"]

AwsTest('Aws can upload file').put_object_with_permission_headers("auth-bucket", "samplefile2", "grant-read-acp" , "id=C12345" ).execute_test().command_is_successful()

########################### No permission in policy and no permission in acl ###########
os.environ["AWS_ACCESS_KEY_ID"] = secondary_access_key
os.environ["AWS_SECRET_ACCESS_KEY"] = secondary_secret_key

policy_authorization_relative = os.path.join(os.path.dirname(__file__), 'policy_files', 'nopermission_policy_nopermission_acl.txt')
policy_authorization = "file://" + os.path.abspath(policy_authorization_relative)

AwsTest("Aws can put policy on bucket").put_bucket_policy("auth-bucket",policy_authorization).execute_test().command_is_successful()

del os.environ["AWS_ACCESS_KEY_ID"]
del os.environ["AWS_SECRET_ACCESS_KEY"]

AwsTest('Unauthorized account can not get object').get_object("auth-bucket", "samplefile")\
.execute_test(negative_case=True).command_should_fail().command_error_should_have("AccessDenied")

############################ Allow-Deny policy and no permission in acl ###################
os.environ["AWS_ACCESS_KEY_ID"] = secondary_access_key
os.environ["AWS_SECRET_ACCESS_KEY"] = secondary_secret_key

policy_authorization_relative = os.path.join(os.path.dirname(__file__), 'policy_files', 'allow_and_deny_policy_nopermission_acl.txt')
policy_authorization = "file://" + os.path.abspath(policy_authorization_relative)

AwsTest("Aws can put policy on bucket").put_bucket_policy("auth-bucket",policy_authorization).execute_test().command_is_successful()

del os.environ["AWS_ACCESS_KEY_ID"]
del os.environ["AWS_SECRET_ACCESS_KEY"]

AwsTest('Aws can upload file').put_object_with_permission_headers("auth-bucket", "samplefile2", "grant-read-acp" , "id=C12345" ).execute_test(negative_case=True).command_should_fail().command_error_should_have("AccessDenied")



############################ nopermission in policy and allow in acl ###################

AwsTest('Aws can get object acl').get_object_acl("auth-bucket", "samplefile").execute_test().command_is_successful()

################## clean up #####################

os.environ["AWS_ACCESS_KEY_ID"] = secondary_access_key
os.environ["AWS_SECRET_ACCESS_KEY"] = secondary_secret_key

AwsTest('Aws can delete object owned by itself').delete_object("auth-bucket","samplefile").execute_test().command_is_successful()
AwsTest('Aws can delete object owned by itself').delete_object("auth-bucket","samplefile2").execute_test().command_is_successful()

AwsTest('AWS can delete bucket seagatebucket02').delete_bucket("auth-bucket").execute_test().command_is_successful()

del os.environ["AWS_ACCESS_KEY_ID"]
del os.environ["AWS_SECRET_ACCESS_KEY"]

AuthTest(test_msg).delete_account(**account_args).execute_test()

print("Authorizing policy tests end....")


# Validate Conditions in Bucket Policy
AwsTest("Aws can put policy on bucket").put_bucket_policy("seagate", policy_condition_StringEquals_success).execute_test().command_is_successful()

AwsTest("Aws can get policy on bucket").get_bucket_policy("seagate").execute_test().command_is_successful().command_response_should_have("s3:x-amz-acl")

AwsTest("Aws can put policy on bucket").put_bucket_policy("seagate", policy_condition_ArnLike_success).execute_test(negative_case=True).command_should_fail().command_error_should_have("MalformedPolicy")

AwsTest("Aws can put policy on bucket").put_bucket_policy("seagate", policy_condition_StringLike_success).execute_test().command_is_successful()

AwsTest("Aws can get policy on bucket").get_bucket_policy("seagate").execute_test().command_is_successful().command_response_should_have("s3:x-amz-acl")

AwsTest("Aws can put policy on bucket").put_bucket_policy("seagate", policy_condition_StringEqualsIfExists_success).execute_test().command_is_successful()

AwsTest("Aws can get policy on bucket").get_bucket_policy("seagate").execute_test().command_is_successful().command_response_should_have("s3:x-amz-acl")

AwsTest("Aws can put policy on bucket").put_bucket_policy("seagate", policy_condition_Bool_success).execute_test().command_is_successful()

AwsTest("Aws can get policy on bucket").get_bucket_policy("seagate").execute_test().command_is_successful().command_response_should_have("aws:SecureTransport")

AwsTest("Aws can put policy on bucket").put_bucket_policy("seagate", policy_condition_Bool_RandomKeyValue_success).execute_test().command_is_successful()

AwsTest("Aws can get policy on bucket").get_bucket_policy("seagate").execute_test().command_is_successful().command_response_should_have("aws:xyz")

AwsTest("Aws can put policy on bucket").put_bucket_policy("seagate", policy_condition_Bool_invalidKey_fail).execute_test(negative_case=True).command_should_fail().command_error_should_have("MalformedPolicy")

AwsTest("Aws can put policy on bucket").put_bucket_policy("seagate", policy_condition_StringEquals_invalidKey_fail).execute_test(negative_case=True).command_should_fail().command_error_should_have("MalformedPolicy")

AwsTest("Aws can put policy on bucket").put_bucket_policy("seagate", policy_condition_NumericLessThanEquals_success).execute_test().command_is_successful()

AwsTest("Aws can get policy on bucket").get_bucket_policy("seagate").execute_test().command_is_successful().command_response_should_have("s3:max-keys")

AwsTest("Aws can put policy on bucket").put_bucket_policy("seagate", policy_condition_DateLessThan_success).execute_test().command_is_successful()

AwsTest("Aws can get policy on bucket").get_bucket_policy("seagate").execute_test().command_is_successful().command_response_should_have("aws:CurrentTime")

AwsTest("Aws can put policy on bucket").put_bucket_policy("seagate", policy_condition_BinaryEquals_success).execute_test(negative_case=True).command_should_fail().command_error_should_have("MalformedPolicy")

AwsTest("Aws can put policy on bucket").put_bucket_policy("seagate", policy_condition_BinaryEquals_invalidValue_fail).execute_test(negative_case=True).command_should_fail().command_error_should_have("MalformedPolicy")

AwsTest("Aws can put policy on bucket").put_bucket_policy("seagate", policy_condition_StringLike_invalidKey_fail).execute_test(negative_case=True).command_should_fail().command_error_should_have("MalformedPolicy")

AwsTest("Aws can put policy on bucket").put_bucket_policy("seagate", policy_condition_StringLikeIfExists_success).execute_test().command_is_successful()

AwsTest("Aws can get policy on bucket").get_bucket_policy("seagate").execute_test().command_is_successful().command_response_should_have("s3:x-amz-acl")

AwsTest("Aws can put policy on bucket").put_bucket_policy("seagate", policy_condition_combination_fail).execute_test(negative_case=True).command_should_fail().command_error_should_have("Conditions do not apply to combination of actions and resources in statement")

AwsTest('Aws can delete bucket seagate').delete_bucket("seagate").execute_test().command_is_successful()
