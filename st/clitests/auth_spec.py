import os
from framework import Config
from framework import S3PyCliTest
from auth import AuthTest
from s3client_config import S3ClientConfig

# Helps debugging
# Config.log_enabled = True
# Config.dummy_run = True

# Set time_readable_format to False if you want to display the time in milli seconds.
# Config.time_readable_format = False

# Store the access keys created during the test.
# These keys should be deleted after
access_key_id = []

# Extract the response elements from response which has the following format
# <Key 1> = <Value 1>, <Key 2> = <Value 2> ... <Key n> = <Value n>
def get_response_elements(response):
    response_elements = {}
    key_pairs = response.split(',')

    for key_pair in key_pairs:
        tokens = key_pair.split('=')
        response_elements[tokens[0].strip()] = tokens[1].strip()

    return response_elements

# Run before all to setup the test environment.
def before_all():
    print("Configuring LDAP")
    S3PyCliTest('Before_all').before_all()

# Test create account API
def account_tests():
    account_args = {}
    account_args['AccountName'] = 's3test'
    account_args['Email'] = 'test@seagate.com'

    test_msg = "Create account s3test"
    account_response_pattern = "AccountId = [\w-]*, CanonicalId = [\w-]*, RootUserName = [\w+=,.@-]*, AccessKeyId = [\w-]*, SecretKey = [\w-]*$"
    result = AuthTest(test_msg).create_account(**account_args).execute_test()
    result.command_should_match_pattern(account_response_pattern)

    account_response_elements = get_response_elements(result.status.stdout)

    # Set S3ClientConfig with root credentials
    S3ClientConfig.access_key_id = account_response_elements['AccessKeyId']
    S3ClientConfig.secret_key = account_response_elements['SecretKey']

    # Add the access key id for clean up
    access_key_id.append(account_response_elements['AccessKeyId'])

    test_msg = "List accounts"
    accounts_response_pattern = "AccountName = [\w-]*, AccountId = [\w-]*, CanonicalId = [\w-]*, Email = [\w.@]*"
    result = AuthTest(test_msg).list_account().execute_test()
    result.command_should_match_pattern(accounts_response_pattern)

# Test create user API
# Case 1 - Path not given (take default value).
# Case 2 - Path given
def user_tests():
    user_args = {}
    user_args['UserName'] = "s3user1"

    test_msg = "Create User s3user1 (default path)"
    user1_response_pattern = "UserId = [\w-]*, ARN = [\S]*, Path = /$"
    result = AuthTest(test_msg).create_user(**user_args).execute_test()
    result.command_should_match_pattern(user1_response_pattern)

    test_msg = "Create User s3user2 (path = /test/)"
    user_args['UserName'] = "s3user2"
    user_args['Path'] = "/test/"
    user2_response_pattern = "UserId = [\w-]*, ARN = [\S]*, Path = /test/$"
    result = AuthTest(test_msg).create_user(**user_args).execute_test()
    result.command_should_match_pattern(user2_response_pattern)

    test_msg = 'Delete User s3user1'
    user_args = {}
    user_args['UserName'] = "s3user1"
    result = AuthTest(test_msg).delete_user(**user_args).execute_test()
    result.command_response_should_have("User deleted.")

    test_msg = 'Delete User s3user2'
    user_args['UserName'] = "s3user2"
    result = AuthTest(test_msg).delete_user(**user_args).execute_test()
    result.command_response_should_have("User deleted.")

    test_msg = 'Update User root (new name = root, new path - /test/success)'
    user_args = {}
    user_args['UserName'] = "root"
    user_args['NewUserName'] = "s3root"
    user_args['NewPath'] = "/test/success/"
    result = AuthTest(test_msg).update_user(**user_args).execute_test()
    result.command_response_should_have("User Updated.")

    test_msg = 'List Users (default path)'
    user_args = {}
    list_user_pattern = "UserId = [\w-]*, UserName = s3root, ARN = [\S]*, Path = /test/success/$"
    result = AuthTest(test_msg).list_users(**user_args).execute_test()
    result.command_should_match_pattern(list_user_pattern)

    test_msg = 'List Users (path prefix = /test/)'
    user_args['PathPrefix'] = '/test/'
    list_user_pattern = "UserId = [\w-]*, UserName = s3root, ARN = [\S]*, Path = /test/success/$"
    result = AuthTest(test_msg).list_users(**user_args).execute_test()
    result.command_should_match_pattern(list_user_pattern)

    test_msg = 'Reset root user attributes (path and name)'
    user_args = {}
    user_args['UserName'] = "s3root"
    user_args['NewUserName'] = "root"
    user_args['NewPath'] = "/"
    result = AuthTest(test_msg).update_user(**user_args).execute_test()
    result.command_response_should_have("User Updated.")


# Test create user API
# Each user can have only 2 access keys. Hence test all the APIs in the same function.
def accesskey_tests():
    access_key_args = {}

    test_msg = 'Create access key (user name not provided)'
    accesskey_response_pattern = "AccessKeyId = [\w-]*, SecretAccessKey = [\w-]*, Status = [\w]*$"
    result = AuthTest(test_msg).create_access_key(**access_key_args).execute_test()
    result.command_should_match_pattern(accesskey_response_pattern)

    accesskey_response_elements = get_response_elements(result.status.stdout)
    access_key_args['AccessKeyId'] = accesskey_response_elements['AccessKeyId']

    test_msg = 'Delete access key'
    result = AuthTest(test_msg).delete_access_key(**access_key_args).execute_test()
    result.command_response_should_have("Access key deleted.")

    test_msg = 'Create access key (user doest not exist.)'
    access_key_args = {}
    access_key_args['UserName'] = 'userDoesNotExist'
    result = AuthTest(test_msg).create_access_key(**access_key_args).execute_test()
    result.command_response_should_have("Exception occured while creating Access Key.")

    test_msg = 'Create access key (user name is root)'
    access_key_args['UserName'] = 'root'
    result = AuthTest(test_msg).create_access_key(**access_key_args).execute_test()
    result.command_should_match_pattern(accesskey_response_pattern)

    accesskey_response_elements = get_response_elements(result.status.stdout)
    access_key_args['AccessKeyId'] = accesskey_response_elements['AccessKeyId']

    test_msg = 'Create access key (Allow only 2 credentials per user.)'
    access_key_args['UserName'] = 'root'
    result = AuthTest(test_msg).create_access_key(**access_key_args).execute_test()
    result.command_response_should_have("Exception occured while creating Access Key.")

    test_msg = 'Delete access key (user name and access key id combination is incorrect)'
    access_key_args['UserName'] = 'root3'
    result = AuthTest(test_msg).delete_access_key(**access_key_args).execute_test()
    result.command_response_should_have("Exception occured while deleting access key.")

    test_msg = 'Update access key (Change status from Active to Inactive)'
    access_key_args['Status'] = "Inactive"
    access_key_args['UserName'] = 'root'
    result = AuthTest(test_msg).update_access_key(**access_key_args).execute_test()
    result.command_response_should_have("Access key Updated.")

    test_msg = 'List access keys (Check if status is inactive.)'
    access_key_args['UserName'] = 'root'
    result = AuthTest(test_msg).list_access_keys(**access_key_args).execute_test()
    result.command_response_should_have("Inactive")

    test_msg = 'Delete access key'
    access_key_args['UserName'] = 'root'
    result = AuthTest(test_msg).delete_access_key(**access_key_args).execute_test()
    result.command_response_should_have("Access key deleted.")

    # List the acess keys to check for status change
    test_msg = 'List access keys'
    access_key_args['UserName'] = 'root'
    accesskey_response_pattern = "UserName = root, AccessKeyId = [\w-]*, Status = Active$"
    result = AuthTest(test_msg).list_access_keys(**access_key_args).execute_test()
    result.command_should_match_pattern(accesskey_response_pattern)

def role_tests():
    policy_doc = os.path.join(os.path.dirname(__file__), 'resources', 'policy')
    policy_doc_full_path = os.path.abspath(policy_doc)

    test_msg = 'Create role (Path not specified)'
    role_args = {}
    role_args['RoleName'] = 'S3Test'
    role_args['AssumeRolePolicyDocument'] = policy_doc_full_path
    role_response_pattern = "RoleId = [\w-]*, RoleName = S3Test, ARN = [\S]*, Path = /$"
    result = AuthTest(test_msg).create_role(**role_args).execute_test()
    result.command_should_match_pattern(role_response_pattern)

    test_msg = 'Delete role'
    result = AuthTest(test_msg).delete_role(**role_args).execute_test()
    result.command_response_should_have("Role deleted.")

    test_msg = 'Create role (Path is /test/)'
    role_args['Path'] = '/test/'
    role_response_pattern = "RoleId = [\w-]*, RoleName = S3Test, ARN = [\S]*, Path = /test/$"
    result = AuthTest(test_msg).create_role(**role_args).execute_test()
    result.command_should_match_pattern(role_response_pattern)

    test_msg = 'List role (Path is not given)'
    role_args = {}
    role_response_pattern = "RoleId = [\w-]*, RoleName = S3Test, ARN = [\S]*, Path = /test/$"
    result = AuthTest(test_msg).list_roles(**role_args).execute_test()
    result.command_should_match_pattern(role_response_pattern)

    test_msg = 'List role (Path is /test)'
    role_response_pattern = "RoleId = S3Test, RoleName = S3Test, ARN = [\S]*, Path = /test/$"
    result = AuthTest(test_msg).list_roles(**role_args).execute_test()
    result.command_should_match_pattern(role_response_pattern)

    test_msg = 'Delete role'
    role_args['RoleName'] = 'S3Test'
    result = AuthTest(test_msg).delete_role(**role_args).execute_test()
    result.command_response_should_have("Role deleted.")

def saml_provider_tests():
    metadata_doc = os.path.join(os.path.dirname(__file__), 'resources', 'saml_metadata')
    metadata_doc_full_path = os.path.abspath(metadata_doc)

    test_msg = 'Create SAML provider'
    saml_provider_args = {}
    saml_provider_args['Name'] = 'S3IDP'
    saml_provider_args['SAMLMetadataDocument'] = metadata_doc_full_path
    saml_provider_response_pattern = "SAMLProviderArn = [\S]*$"
    result = AuthTest(test_msg).create_saml_provider(**saml_provider_args).execute_test()
    result.command_should_match_pattern(saml_provider_response_pattern)

    response_elements = get_response_elements(result.status.stdout)
    saml_provider_args['SAMLProviderArn'] = response_elements['SAMLProviderArn']

    test_msg = 'Update SAML provider'
    saml_provider_args = {}
    saml_provider_args['SAMLProviderArn'] = "arn:seagate:iam:::S3IDP"
    saml_provider_args['SAMLMetadataDocument'] = metadata_doc_full_path
    result = AuthTest(test_msg).update_saml_provider(**saml_provider_args).execute_test()
    result.command_response_should_have("SAML provider Updated.")

    test_msg = 'List SAML providers'
    saml_provider_response_pattern = "ARN = arn:seagate:iam:::S3IDP, ValidUntil = [\S\s]*$"
    result = AuthTest(test_msg).list_saml_providers(**saml_provider_args).execute_test()
    result.command_should_match_pattern(saml_provider_response_pattern)

    test_msg = 'Delete SAML provider'
    result = AuthTest(test_msg).delete_saml_provider(**saml_provider_args).execute_test()
    result.command_response_should_have("SAML provider deleted.")

    test_msg = 'List SAML providers'
    result = AuthTest(test_msg).list_saml_providers(**saml_provider_args).execute_test()
    result.command_should_match_pattern("")

def get_federation_token_test():
    federation_token_args = {}
    federation_token_args['Name'] = 's3root'

    test_msg = 'Get Federation Token'
    response_pattern = "FederatedUserId = [\S]*, AccessKeyId = [\w-]*, SecretAccessKey = [\w-]*, SessionToken = [\w-]*$"
    result = AuthTest(test_msg).get_federation_token(**federation_token_args).execute_test()
    result.command_should_match_pattern(response_pattern)

    response_elements = get_response_elements(result.status.stdout)
    S3ClientConfig.access_key_id = response_elements['AccessKeyId']
    S3ClientConfig.secret_key = response_elements['SecretAccessKey']
    S3ClientConfig.token = response_elements['SessionToken']

    test_msg = 'List Users (default path)'
    user_args = {}
    list_user_pattern = "UserId = [\w-]*, UserName = root, ARN = [\S]*, Path = /$"
    result = AuthTest(test_msg).list_users(**user_args).execute_test()
    result.command_should_match_pattern(list_user_pattern)

#  ***MAIN ENTRY POINT
# Do not change the order.
before_all()
account_tests()
user_tests()
accesskey_tests()
role_tests()
saml_provider_tests()
get_federation_token_test()
