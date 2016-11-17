import os
from framework import Config
from framework import S3PyCliTest
from auth import AuthTest
from s3client_config import S3ClientConfig
from ldap import LdapOps

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

# Test create account and list accounts APIs
def account_tests():
    account_args = {}
    account_args['AccountName'] = 's3test'
    account_args['Email'] = 'test@seagate.com'

    # CREATE
    AuthTest("Set LDAP_ADD_ENTRY_FAIL fault point").inject_fault("LDAP_ADD_ENTRY_FAIL", "FAIL_ONCE", 0).execute_test()
    test_msg = "Create account s3test should fail if save account details fails"
    AuthTest(test_msg).create_account(**account_args).execute_test()\
            .command_response_should_have("Internal Server Error")
    AuthTest("Reset LDAP_ADD_ENTRY_FAIL fault point").reset_fault("LDAP_ADD_ENTRY_FAIL").execute_test()
    LdapOps().delete_account("s3test")

    AuthTest("Set LDAP_ADD_ENTRY_FAIL fault point").inject_fault("LDAP_ADD_ENTRY_FAIL", "SKIP_FIRST_N_TIMES", 1).execute_test()
    test_msg = "Create account s3test should fail when create user ou fails"
    AuthTest(test_msg).create_account(**account_args).execute_test()\
            .command_response_should_have("Internal Server Error")
    AuthTest("Reset LDAP_ADD_ENTRY_FAIL fault point").reset_fault("LDAP_ADD_ENTRY_FAIL").execute_test()
    LdapOps().delete_account("s3test")

    AuthTest("Set LDAP_ADD_ENTRY_FAIL fault point").inject_fault("LDAP_ADD_ENTRY_FAIL", "SKIP_FIRST_N_TIMES", 2).execute_test()
    test_msg = "Create account s3test should fail when create role ou fails"
    AuthTest(test_msg).create_account(**account_args).execute_test()\
            .command_response_should_have("Internal Server Error")
    AuthTest("Reset LDAP_ADD_ENTRY_FAIL fault point").reset_fault("LDAP_ADD_ENTRY_FAIL").execute_test()
    LdapOps().delete_account("s3test")

    AuthTest("Set LDAP_ADD_ENTRY_FAIL fault point").inject_fault("LDAP_ADD_ENTRY_FAIL", "SKIP_FIRST_N_TIMES", 3).execute_test()
    test_msg = "Create account s3test should fail when create groups ou fails"
    AuthTest(test_msg).create_account(**account_args).execute_test()\
            .command_response_should_have("Internal Server Error")
    AuthTest("Reset LDAP_ADD_ENTRY_FAIL fault point").reset_fault("LDAP_ADD_ENTRY_FAIL").execute_test()
    LdapOps().delete_account("s3test")

    AuthTest("Set LDAP_ADD_ENTRY_FAIL fault point").inject_fault("LDAP_ADD_ENTRY_FAIL", "SKIP_FIRST_N_TIMES", 4).execute_test()
    test_msg = "Create account s3test should fail when create policy ou fails"
    AuthTest(test_msg).create_account(**account_args).execute_test()\
            .command_response_should_have("Internal Server Error")
    AuthTest("Reset LDAP_ADD_ENTRY_FAIL fault point").reset_fault("LDAP_ADD_ENTRY_FAIL").execute_test()
    LdapOps().delete_account("s3test")

    AuthTest("Set LDAP_ADD_ENTRY_FAIL fault point").inject_fault("LDAP_ADD_ENTRY_FAIL", "SKIP_FIRST_N_TIMES", 5).execute_test()
    test_msg = "Create account s3test should fail when create root user account fails"
    AuthTest(test_msg).create_account(**account_args).execute_test()\
            .command_response_should_have("Internal Server Error")
    AuthTest("Reset LDAP_ADD_ENTRY_FAIL fault point").reset_fault("LDAP_ADD_ENTRY_FAIL").execute_test()
    LdapOps().delete_account("s3test")

    AuthTest("Set LDAP_ADD_ENTRY_FAIL fault point").inject_fault("LDAP_ADD_ENTRY_FAIL", "SKIP_FIRST_N_TIMES", 6).execute_test()
    test_msg = "Create account s3test should fail when create access key for root user account fails"
    AuthTest(test_msg).create_account(**account_args).execute_test()\
            .command_response_should_have("Internal Server Error")
    AuthTest("Reset LDAP_ADD_ENTRY_FAIL fault point").reset_fault("LDAP_ADD_ENTRY_FAIL").execute_test()
    LdapOps().delete_account("s3test")

    # Create Account
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

    test_msg = "Create account s3test should fail if the account already exist"
    AuthTest(test_msg).create_account(**account_args).execute_test()\
            .command_response_should_have("Conflict")

    # LIST
    test_msg = "List accounts"
    accounts_response_pattern = "AccountName = [\w-]*, AccountId = [\w-]*, CanonicalId = [\w-]*, Email = [\w.@]*"
    AuthTest(test_msg).list_account().execute_test()\
            .command_should_match_pattern(accounts_response_pattern)

    # Fail to search account while creating account
    AuthTest("Set LDAP_SEARCH_FAIL fault point").inject_fault("LDAP_SEARCH_FAIL", "FAIL_ALWAYS", 0).execute_test()
    test_msg = "Create account s3test should fail if search account fails"
    AuthTest(test_msg).create_account(**account_args).execute_test()\
            .command_response_should_have("Internal Server Error")
    test_msg = "List accounts should fail if search accounts fails"
    AuthTest(test_msg).list_account().execute_test()\
            .command_response_should_have("Internal Server Error")
    AuthTest("Reset LDAP_SEARCH_FAIL fault point").reset_fault("LDAP_SEARCH_FAIL").execute_test()

    # Fail to get attrubutes of searched account while creating account
    AuthTest("Set LDAP_GET_ATTR_FAIL fault point").inject_fault("LDAP_GET_ATTR_FAIL", "FAIL_ALWAYS", 0).execute_test()
    test_msg = "Create account s3test should fail if find attributes of searched account fails"
    AuthTest(test_msg).create_account(**account_args).execute_test()\
            .command_response_should_have("Internal Server Error")
    test_msg = "List accounts should fail if find attributes of searched account fails"
    AuthTest(test_msg).list_account().execute_test()\
            .command_response_should_have("Internal Server Error")
    AuthTest("Reset LDAP_GET_ATTR_FAIL fault point").reset_fault("LDAP_GET_ATTR_FAIL").execute_test()

def user_tests():
    user_args = {}
    user_args['UserName'] = "s3user1"

    # CREATE
    AuthTest("Set LDAP_SEARCH_FAIL fault point").inject_fault("LDAP_SEARCH_FAIL", "SKIP_FIRST_N_TIMES", 3).execute_test()
    test_msg = "Create user s3user1 should fail if search user fails"
    AuthTest(test_msg).create_user(**user_args).execute_test()\
            .command_response_should_have("Exception occured while creating user")
    AuthTest("Reset LDAP_SEARCH_FAIL fault point").reset_fault("LDAP_SEARCH_FAIL").execute_test()

    AuthTest("Set LDAP_ADD_ENTRY_FAIL fault point").inject_fault("LDAP_ADD_ENTRY_FAIL", "FAIL_ALWAYS", 0).execute_test()
    test_msg = "Create user should fail if save user fails"
    AuthTest(test_msg).create_user(**user_args).execute_test()\
            .command_response_should_have("Exception occured while creating user")
    AuthTest("Reset LDAP_ADD_ENTRY_FAIL fault point").reset_fault("LDAP_ADD_ENTRY_FAIL").execute_test()

    test_msg = "Create User s3user1 (default path)"
    user1_response_pattern = "UserId = [\w-]*, ARN = [\S]*, Path = /$"
    result = AuthTest(test_msg).create_user(**user_args).execute_test()
    result.command_should_match_pattern(user1_response_pattern)

    test_msg = "Create user should fail if user already exist"
    AuthTest(test_msg).create_user(**user_args).execute_test()\
            .command_response_should_have("Exception occured while creating user")

    # LIST
    AuthTest("Set LDAP_SEARCH_FAIL fault point").inject_fault("LDAP_SEARCH_FAIL", "SKIP_FIRST_N_TIMES", 3).execute_test()
    test_msg = "List users should fail if find users fails"
    user_args = {}
    AuthTest(test_msg).list_users(**user_args).execute_test()\
            .command_response_should_have("Exception occured while listing users")
    AuthTest("Reset LDAP_SEARCH_FAIL fault point").reset_fault("LDAP_SEARCH_FAIL").execute_test()

    # UPDATE
    AuthTest("Set LDAP_UPDATE_ENTRY_FAIL fault point").inject_fault("LDAP_UPDATE_ENTRY_FAIL", "FAIL_ALWAYS", 0).execute_test()
    test_msg = "Update user should fail if update fails"
    user_args = {}
    user_args['UserName'] = "s3user1"
    user_args['NewUserName'] = "s3user11"
    AuthTest(test_msg).update_user(**user_args).execute_test()\
            .command_response_should_have("Exception occured while updating user info")
    AuthTest("Reset LDAP_UPDATE_ENTRY_FAIL fault point").reset_fault("LDAP_UPDATE_ENTRY_FAIL").execute_test()

    test_msg = "Update user should fail if no parameters passed"
    user_args = {}
    user_args['UserName'] = "s3user1"
    AuthTest(test_msg).update_user(**user_args).execute_test()\
            .command_response_should_have("Exception occured while updating user info")

    test_msg = "Update user fails if new username and old username are same"
    user_args = {}
    user_args['UserName'] = "s3user1"
    user_args['NewUserName'] = "s3user1"
    AuthTest(test_msg).update_user(**user_args).execute_test()\
            .command_response_should_have("Exception occured while updating user info")

    AuthTest("Set LDAP_SEARCH_FAIL fault point").inject_fault("LDAP_SEARCH_FAIL", "SKIP_FIRST_N_TIMES", 3).execute_test()
    test_msg = 'Update User s3user1 (new name = root, new path - /test/success) should fail'
    user_args = {}
    user_args['UserName'] = "s3user1"
    user_args['NewUserName'] = "s3userA"
    user_args['NewPath'] = "/test/success/"
    AuthTest(test_msg).update_user(**user_args).execute_test()\
            .command_response_should_have("Exception occured while updating user info")
    AuthTest("Reset LDAP_SEARCH_FAIL fault point").reset_fault("LDAP_SEARCH_FAIL").execute_test()

    AuthTest("Set LDAP_SEARCH_FAIL fault point").inject_fault("LDAP_SEARCH_FAIL", "SKIP_FIRST_N_TIMES", 4).execute_test()
    test_msg = 'Update User s3user1 (new name = root, new path - /test/success) should fail'
    user_args = {}
    user_args['UserName'] = "s3user1"
    user_args['NewUserName'] = "s3userA"
    user_args['NewPath'] = "/test/success/"
    AuthTest(test_msg).update_user(**user_args).execute_test()\
            .command_response_should_have("Exception occured while updating user info")
    AuthTest("Reset LDAP_SEARCH_FAIL fault point").reset_fault("LDAP_SEARCH_FAIL").execute_test()

    test_msg = "Update user should fail if user doesn't exist"
    user_args = {}
    user_args['UserName'] = "noSuchUser"
    user_args['NewUserName'] = "s3userA"
    user_args['NewPath'] = "/test/success/"
    AuthTest(test_msg).update_user(**user_args).execute_test()\
            .command_response_should_have("Exception occured while updating user info")

    # DELETE
    AuthTest("Set LDAP_SEARCH_FAIL fault point").inject_fault("LDAP_SEARCH_FAIL", "SKIP_FIRST_N_TIMES", 3).execute_test()
    test_msg = "Delete user s3user1 should fail if find user fails"
    user_args['UserName'] = "s3user1"
    AuthTest(test_msg).delete_user(**user_args).execute_test()\
            .command_response_should_have("Exception occured while deleting user")
    AuthTest("Reset LDAP_SEARCH_FAIL fault point").reset_fault("LDAP_SEARCH_FAIL").execute_test()

    # Fail to delete user
    AuthTest("Set LDAP_DELETE_ENTRY_FAIL fault point").inject_fault("LDAP_DELETE_ENTRY_FAIL", "FAIL_ALWAYS", 0).execute_test()
    test_msg = "Delete user should fail if delete operation fails"
    user_args['UserName'] = "s3user1"
    AuthTest(test_msg).delete_user(**user_args).execute_test()\
            .command_response_should_have("Exception occured while deleting user")
    AuthTest("Reset LDAP_DELETE_ENTRY_FAIL fault point").reset_fault("LDAP_DELETE_ENTRY_FAIL").execute_test()


    test_msg = "Create access key for user s3user1"
    access_key_args = {}
    access_key_args['UserName'] = 's3user1'
    accesskey_response_pattern = "AccessKeyId = [\w-]*, SecretAccessKey = [\w-]*, Status = [\w]*$"
    result = AuthTest(test_msg).create_access_key(**access_key_args).execute_test()
    result.command_should_match_pattern(accesskey_response_pattern)
    accesskey_response_elements = get_response_elements(result.status.stdout)
    access_key_args['AccessKeyId'] = accesskey_response_elements['AccessKeyId']

    test_msg = "Delete user s3user1 should fail when it has access key"
    AuthTest(test_msg).delete_user(**user_args).execute_test()\
            .command_response_should_have("Exception occured while deleting user")

    test_msg = "Delete access key of s3user1"
    AuthTest(test_msg).delete_access_key(**access_key_args).execute_test()\
            .command_response_should_have("Access key deleted")

    test_msg = "Delete user s3user1 should succeed"
    user_args['UserName'] = "s3user1"
    AuthTest(test_msg).delete_user(**user_args).execute_test()\
            .command_response_should_have("User deleted")

    test_msg = "Delete user fails when user does not exist"
    user_args['UserName'] = "s3user1"
    AuthTest(test_msg).delete_user(**user_args).execute_test()\
            .command_response_should_have("Exception occured while deleting user")

def create_test_user():
    test_msg = "Create User s3user2 (path = /test/)"
    user_args = {}
    user_args['UserName'] = "s3user2"
    user_args['Path'] = "/test/"
    user2_response_pattern = "UserId = [\w-]*, ARN = [\S]*, Path = /test/$"
    result = AuthTest(test_msg).create_user(**user_args).execute_test()
    result.command_should_match_pattern(user2_response_pattern)

def delete_test_user():
    test_msg = 'Delete User s3user2'
    user_args = {}
    user_args['UserName'] = "s3user2"
    result = AuthTest(test_msg).delete_user(**user_args).execute_test()
    result.command_response_should_have("User deleted.")


# Test create user API
# Each user can have only 2 access keys. Hence test all the APIs in the same function.
def accesskey_tests():
    create_test_user()
    access_key_args = {}

    # CREATE
    AuthTest("Set LDAP_SEARCH_FAIL fault point").inject_fault("LDAP_SEARCH_FAIL", "SKIP_FIRST_N_TIMES", 3).execute_test()
    test_msg = 'Create access key (user name is root) should fail'
    access_key_args['UserName'] = 'root'
    result = AuthTest(test_msg).create_access_key(**access_key_args).execute_test()\
            .command_should_match_pattern("Exception occured while creating Access Key")
    AuthTest("Reset LDAP_SEARCH_FAIL fault point").reset_fault("LDAP_SEARCH_FAIL").execute_test()

    AuthTest("Set LDAP_SEARCH_FAIL fault point").inject_fault("LDAP_SEARCH_FAIL", "SKIP_FIRST_N_TIMES", 4).execute_test()
    test_msg = 'Create access key (user name is root) should fail'
    access_key_args['UserName'] = 'root'
    result = AuthTest(test_msg).create_access_key(**access_key_args).execute_test()\
            .command_should_match_pattern("Exception occured while creating Access Key")
    AuthTest("Reset LDAP_SEARCH_FAIL fault point").reset_fault("LDAP_SEARCH_FAIL").execute_test()

    AuthTest("Set LDAP_ADD_ENTRY_FAIL fault point").inject_fault("LDAP_ADD_ENTRY_FAIL", "FAIL_ALWAYS", 0).execute_test()
    test_msg = 'Create access key (user name is root) should fail'
    access_key_args['UserName'] = 'root'
    result = AuthTest(test_msg).create_access_key(**access_key_args).execute_test()\
            .command_should_match_pattern("Exception occured while creating Access Key")
    AuthTest("Reset LDAP_ADD_ENTRY_FAIL fault point").reset_fault("LDAP_ADD_ENTRY_FAIL").execute_test()

    test_msg = 'Create access key (user name is root)'
    access_key_args['UserName'] = 'root'
    accesskey_response_pattern = "AccessKeyId = [\w-]*, SecretAccessKey = [\w-]*, Status = [\w]*$"
    result = AuthTest(test_msg).create_access_key(**access_key_args).execute_test()
    result.command_should_match_pattern(accesskey_response_pattern)

    accesskey_response_elements = get_response_elements(result.status.stdout)
    access_key_args['AccessKeyId'] = accesskey_response_elements['AccessKeyId']

    # LIST
    test_msg = 'List access keys should without passing username'
    access_key_args.pop('UserName', None)
    accesskey_response_pattern = "UserName = root, AccessKeyId = [\w-]*, Status = Active"
    AuthTest(test_msg).list_access_keys(**access_key_args).execute_test()\
            .command_should_match_pattern(accesskey_response_pattern)

    AuthTest("Set LDAP_SEARCH_FAIL fault point").inject_fault("LDAP_SEARCH_FAIL", "SKIP_FIRST_N_TIMES", 3).execute_test()
    test_msg = 'List access keys should fail'
    access_key_args['UserName'] = 'root'
    AuthTest(test_msg).list_access_keys(**access_key_args).execute_test()\
            .command_response_should_have("Exception occured while listing access keys")
    AuthTest("Reset LDAP_SEARCH_FAIL fault point").reset_fault("LDAP_SEARCH_FAIL").execute_test()

    test_msg = "List access keys should fail if user doesn't exist"
    access_key_args['UserName'] = 'noSuchUser'
    AuthTest(test_msg).list_access_keys(**access_key_args).execute_test()\
            .command_response_should_have("Exception occured while listing access keys")

    AuthTest("Set LDAP_SEARCH_FAIL fault point").inject_fault("LDAP_SEARCH_FAIL", "SKIP_FIRST_N_TIMES", 4).execute_test()
    test_msg = 'List access keys should fail'
    access_key_args['UserName'] = 'root'
    AuthTest(test_msg).list_access_keys(**access_key_args).execute_test()\
            .command_response_should_have("Exception occured while listing access keys")
    AuthTest("Reset LDAP_SEARCH_FAIL fault point").reset_fault("LDAP_SEARCH_FAIL").execute_test()

    # UPDATE
    AuthTest("Set LDAP_SEARCH_FAIL fault point").inject_fault("LDAP_SEARCH_FAIL", "SKIP_FIRST_N_TIMES", 3).execute_test()
    test_msg = 'Update access key should fail'
    access_key_args['Status'] = "Inactive"
    access_key_args['UserName'] = 'root'
    AuthTest(test_msg).update_access_key(**access_key_args).execute_test()\
            .command_response_should_have("Exception occured while updating access key")
    AuthTest("Reset LDAP_SEARCH_FAIL fault point").reset_fault("LDAP_SEARCH_FAIL").execute_test()

    AuthTest("Set LDAP_SEARCH_FAIL fault point").inject_fault("LDAP_SEARCH_FAIL", "SKIP_FIRST_N_TIMES", 4).execute_test()
    test_msg = 'Update access key should fail'
    access_key_args['Status'] = "Inactive"
    access_key_args['UserName'] = 'root'
    AuthTest(test_msg).update_access_key(**access_key_args).execute_test()\
            .command_response_should_have("Exception occured while updating access key")
    AuthTest("Reset LDAP_SEARCH_FAIL fault point").reset_fault("LDAP_SEARCH_FAIL").execute_test()

    AuthTest("Set LDAP_UPDATE_ENTRY_FAIL fault point").inject_fault("LDAP_UPDATE_ENTRY_FAIL", "FAIL_ALWAYS", 0).execute_test()
    test_msg = 'Update access key should fail'
    access_key_args['Status'] = "Inactive"
    access_key_args['UserName'] = 'root'
    AuthTest(test_msg).update_access_key(**access_key_args).execute_test()\
            .command_response_should_have("Exception occured while updating access key")
    AuthTest("Reset LDAP_UPDATE_ENTRY_FAIL fault point").reset_fault("LDAP_UPDATE_ENTRY_FAIL").execute_test()

    test_msg = "Update access key should fail if user doesn't exist"
    access_key_args['Status'] = "Inactive"
    access_key_args['UserName'] = 'NoSuchUser'
    AuthTest(test_msg).update_access_key(**access_key_args).execute_test()\
            .command_response_should_have("Exception occured while updating access key")

    test_msg = 'Update access key should fail if user is invalid'
    access_key_args['Status'] = "Inactive"
    access_key_args['UserName'] = 's3user2'
    AuthTest(test_msg).update_access_key(**access_key_args).execute_test()\
            .command_response_should_have("Exception occured while updating access key")

    test_msg = 'Update access key should fail if access key is invalid'
    access_key_args['Status'] = "Inactive"
    access_key_args['UserName'] = 'root'
    ak_holder = access_key_args['AccessKeyId']
    access_key_args['AccessKeyId'] = "NO-SUCH-ACCESS-KEY"
    AuthTest(test_msg).update_access_key(**access_key_args).execute_test()\
            .command_response_should_have("Exception occured while updating access key")
    access_key_args['AccessKeyId'] = ak_holder

    # DELETE
    AuthTest("Set LDAP_SEARCH_FAIL fault point").inject_fault("LDAP_SEARCH_FAIL", "SKIP_FIRST_N_TIMES", 3).execute_test()
    test_msg = 'Delete access key should fail'
    access_key_args['UserName'] = 'root'
    result = AuthTest(test_msg).delete_access_key(**access_key_args).execute_test()\
            .command_response_should_have("Exception occured while deleting access key")
    AuthTest("Reset LDAP_SEARCH_FAIL fault point").reset_fault("LDAP_SEARCH_FAIL").execute_test()

    AuthTest("Set LDAP_SEARCH_FAIL fault point").inject_fault("LDAP_SEARCH_FAIL", "SKIP_FIRST_N_TIMES", 4).execute_test()
    test_msg = 'Delete access key should fail'
    access_key_args['UserName'] = 'root'
    result = AuthTest(test_msg).delete_access_key(**access_key_args).execute_test()\
            .command_response_should_have("Exception occured while deleting access key")
    AuthTest("Reset LDAP_SEARCH_FAIL fault point").reset_fault("LDAP_SEARCH_FAIL").execute_test()

    test_msg = 'Delete access key should fail if username is wrong'
    access_key_args['UserName'] = 's3user2'
    result = AuthTest(test_msg).delete_access_key(**access_key_args).execute_test()\
            .command_response_should_have("Exception occured while deleting access key")

    AuthTest("Set LDAP_DELETE_ENTRY_FAIL fault point").inject_fault("LDAP_DELETE_ENTRY_FAIL", "FAIL_ALWAYS", 0).execute_test()
    test_msg = 'Delete access key should fail if delete entry fails'
    access_key_args['UserName'] = 'root'
    result = AuthTest(test_msg).delete_access_key(**access_key_args).execute_test()\
            .command_response_should_have("Exception occured while deleting access key")
    AuthTest("Reset LDAP_DELETE_ENTRY_FAIL fault point").reset_fault("LDAP_DELETE_ENTRY_FAIL").execute_test()

    test_msg = 'Delete access key'
    access_key_args['UserName'] = 'root'
    result = AuthTest(test_msg).delete_access_key(**access_key_args).execute_test()\
            .command_response_should_have("Access key deleted.")

    test_msg = 'Delete access key should fail if access key is invalid'
    access_key_args['UserName'] = 'root'
    access_key_args['AccessKeyId'] = "INVALID-ACCESS-KEY"
    result = AuthTest(test_msg).delete_access_key(**access_key_args).execute_test()\
            .command_response_should_have("Exception occured while deleting access key")

    delete_test_user()

def role_tests():
    policy_doc = os.path.join(os.path.dirname(__file__), 'resources', 'policy')
    policy_doc_full_path = os.path.abspath(policy_doc)

    # CREATE
    AuthTest("Set LDAP_SEARCH_FAIL fault point").inject_fault("LDAP_SEARCH_FAIL", "SKIP_FIRST_N_TIMES", 3).execute_test()
    test_msg = 'Create role (Path not specified) should fail'
    role_args = {}
    role_args['RoleName'] = 'S3Test'
    role_args['AssumeRolePolicyDocument'] = policy_doc_full_path
    AuthTest(test_msg).create_role(**role_args).execute_test()\
            .command_response_should_have("Exception occured while creating role")
    AuthTest("Reset LDAP_SEARCH_FAIL fault point").reset_fault("LDAP_SEARCH_FAIL").execute_test()

    AuthTest("Set LDAP_ADD_ENTRY_FAIL fault point").inject_fault("LDAP_ADD_ENTRY_FAIL", "FAIL_ALWAYS", 0).execute_test()
    test_msg = 'Create role (Path not specified) should fail if save role failed'
    role_args = {}
    role_args['RoleName'] = 'S3Test'
    role_args['AssumeRolePolicyDocument'] = policy_doc_full_path
    AuthTest(test_msg).create_role(**role_args).execute_test()\
            .command_response_should_have("Exception occured while creating role")
    AuthTest("Reset LDAP_ADD_ENTRY_FAIL fault point").reset_fault("LDAP_ADD_ENTRY_FAIL").execute_test()

    test_msg = 'Create role (Path not specified)'
    role_args = {}
    role_args['RoleName'] = 'S3Test'
    role_args['AssumeRolePolicyDocument'] = policy_doc_full_path
    role_response_pattern = "RoleId = [\w-]*, RoleName = S3Test, ARN = [\S]*, Path = /$"
    AuthTest(test_msg).create_role(**role_args).execute_test()\
            .command_should_match_pattern(role_response_pattern)

    test_msg = 'Create role (Path not specified) should fail if role already exist'
    role_args = {}
    role_args['RoleName'] = 'S3Test'
    role_args['AssumeRolePolicyDocument'] = policy_doc_full_path
    role_response_pattern = "RoleId = [\w-]*, RoleName = S3Test, ARN = [\S]*, Path = /$"
    AuthTest(test_msg).create_role(**role_args).execute_test()\
            .command_response_should_have("Exception occured while creating role")

    # LIST
    AuthTest("Set LDAP_SEARCH_FAIL fault point").inject_fault("LDAP_SEARCH_FAIL", "SKIP_FIRST_N_TIMES", 3).execute_test()
    test_msg = 'List roles should fail'
    role_args = {}
    role_args['RoleName'] = 'S3Test'
    AuthTest(test_msg).list_roles(**role_args).execute_test()\
            .command_response_should_have("Exception occured while listing roles")
    AuthTest("Reset LDAP_SEARCH_FAIL fault point").reset_fault("LDAP_SEARCH_FAIL").execute_test()

    AuthTest("Set LDAP_SEARCH_FAIL fault point").inject_fault("LDAP_SEARCH_FAIL", "SKIP_FIRST_N_TIMES", 3).execute_test()
    test_msg = "Delete role should fail"
    AuthTest(test_msg).delete_role(**role_args).execute_test()\
            .command_response_should_have("Exception occured while deleting role")
    AuthTest("Reset LDAP_SEARCH_FAIL fault point").reset_fault("LDAP_SEARCH_FAIL").execute_test()

    # DELETE
    AuthTest("Set LDAP_DELETE_ENTRY_FAIL fault point").inject_fault("LDAP_DELETE_ENTRY_FAIL", "FAIL_ALWAYS", 0).execute_test()
    test_msg = "Delete role should fail"
    AuthTest(test_msg).delete_role(**role_args).execute_test()\
            .command_response_should_have("Exception occured while deleting role")
    AuthTest("Reset LDAP_DELETE_ENTRY_FAIL fault point").reset_fault("LDAP_DELETE_ENTRY_FAIL").execute_test()

    test_msg = 'Delete role'
    AuthTest(test_msg).delete_role(**role_args).execute_test()\
            .command_response_should_have("Role deleted.")

    test_msg = "Delete role should fail if role doesn't exist"
    AuthTest(test_msg).delete_role(**role_args).execute_test()\
            .command_response_should_have("Exception occured while deleting role")

    # remove following test by making approp. changes in auth_spec
    test_msg = 'Create role (Path is /test/)'
    role_args['RoleName'] = 'S3Test'
    role_args['Path'] = '/test/'
    role_args['AssumeRolePolicyDocument'] = policy_doc_full_path
    role_response_pattern = "RoleId = [\w-]*, RoleName = S3Test, ARN = [\S]*, Path = /test/$"
    AuthTest(test_msg).create_role(**role_args).execute_test()\
            .command_should_match_pattern(role_response_pattern)

    test_msg = 'List role (Path is /test)'
    role_args = {}
    role_args['Path'] = '/test/'
    role_response_pattern = "RoleId = S3Test, RoleName = S3Test, ARN = [\S]*, Path = /test/$"
    AuthTest(test_msg).list_roles(**role_args).execute_test()\
            .command_should_match_pattern(role_response_pattern)

    test_msg = 'Delete role'
    role_args['RoleName'] = 'S3Test'
    AuthTest(test_msg).delete_role(**role_args).execute_test()\
            .command_response_should_have("Role deleted.")

def saml_provider_tests():
    metadata_doc = os.path.join(os.path.dirname(__file__), 'resources', 'saml_metadata')
    metadata_doc_full_path = os.path.abspath(metadata_doc)

    # CREATE
    AuthTest("Set LDAP_SEARCH_FAIL fault point").inject_fault("LDAP_SEARCH_FAIL", "SKIP_FIRST_N_TIMES", 3).execute_test()
    test_msg = 'Create SAML provider should fail'
    saml_provider_args = {}
    saml_provider_args['Name'] = 'S3IDP'
    saml_provider_args['SAMLMetadataDocument'] = metadata_doc_full_path
    AuthTest(test_msg).create_saml_provider(**saml_provider_args).execute_test()\
            .command_response_should_have("Exception occured while creating saml provider")
    AuthTest("Reset LDAP_SEARCH_FAIL fault point").reset_fault("LDAP_SEARCH_FAIL").execute_test()

    AuthTest("Set LDAP_ADD_ENTRY_FAIL fault point").inject_fault("LDAP_ADD_ENTRY_FAIL", "FAIL_ALWAYS", 0).execute_test()
    test_msg = 'Create SAML provider should fail'
    saml_provider_args = {}
    saml_provider_args['Name'] = 'S3IDP'
    saml_provider_args['SAMLMetadataDocument'] = metadata_doc_full_path
    AuthTest(test_msg).create_saml_provider(**saml_provider_args).execute_test()\
            .command_response_should_have("Exception occured while creating saml provider")
    AuthTest("Reset LDAP_ADD_ENTRY_FAIL fault point").reset_fault("LDAP_ADD_ENTRY_FAIL").execute_test()

    test_msg = 'Create SAML provider'
    saml_provider_args = {}
    saml_provider_args['Name'] = 'S3IDP'
    saml_provider_args['SAMLMetadataDocument'] = metadata_doc_full_path
    saml_provider_response_pattern = "SAMLProviderArn = [\S]*$"
    result = AuthTest(test_msg).create_saml_provider(**saml_provider_args).execute_test()
    result.command_should_match_pattern(saml_provider_response_pattern)

    response_elements = get_response_elements(result.status.stdout)
    saml_provider_args['SAMLProviderArn'] = response_elements['SAMLProviderArn']

    test_msg = 'Create SAML provider should fail if it already exists'
    saml_provider_args = {}
    saml_provider_args['Name'] = 'S3IDP'
    saml_provider_args['SAMLMetadataDocument'] = metadata_doc_full_path
    AuthTest(test_msg).create_saml_provider(**saml_provider_args).execute_test()\
            .command_response_should_have("Exception occured while creating saml provider")

    # LIST
    AuthTest("Set LDAP_SEARCH_FAIL fault point").inject_fault("LDAP_SEARCH_FAIL", "SKIP_FIRST_N_TIMES", 3).execute_test()
    test_msg = 'List SAML providers'
    saml_provider_args = {}
    AuthTest(test_msg).list_saml_providers(**saml_provider_args).execute_test()\
            .command_response_should_have("Exception occured while listing SAML providers")
    AuthTest("Reset LDAP_SEARCH_FAIL fault point").reset_fault("LDAP_SEARCH_FAIL").execute_test()

    # UPDATE
    AuthTest("Set LDAP_SEARCH_FAIL fault point").inject_fault("LDAP_SEARCH_FAIL", "SKIP_FIRST_N_TIMES", 3).execute_test()
    test_msg = 'Update SAML provider should fail'
    saml_provider_args = {}
    saml_provider_args['SAMLProviderArn'] = "arn:seagate:iam:::S3IDP"
    saml_provider_args['SAMLMetadataDocument'] = metadata_doc_full_path
    AuthTest(test_msg).update_saml_provider(**saml_provider_args).execute_test()\
            .command_response_should_have("Exception occured while updating SAML provider")
    AuthTest("Reset LDAP_SEARCH_FAIL fault point").reset_fault("LDAP_SEARCH_FAIL").execute_test()

    test_msg = 'Update SAML provider should fail if provider is invalid'
    saml_provider_args = {}
    saml_provider_args['SAMLProviderArn'] = "arn:seagate:iam:::S3INVALID"
    saml_provider_args['SAMLMetadataDocument'] = metadata_doc_full_path
    AuthTest(test_msg).update_saml_provider(**saml_provider_args).execute_test()\
            .command_response_should_have("Exception occured while updating SAML provider")

    AuthTest("Set LDAP_UPDATE_ENTRY_FAIL fault point").inject_fault("LDAP_UPDATE_ENTRY_FAIL", "FAIL_ALWAYS", 0).execute_test()
    test_msg = 'Update SAML provider should fail'
    saml_provider_args = {}
    saml_provider_args['SAMLProviderArn'] = "arn:seagate:iam:::S3IDP"
    saml_provider_args['SAMLMetadataDocument'] = metadata_doc_full_path
    AuthTest(test_msg).update_saml_provider(**saml_provider_args).execute_test()\
            .command_response_should_have("Exception occured while updating SAML provider")
    AuthTest("Reset LDAP_UPDATE_ENTRY_FAIL fault point").reset_fault("LDAP_UPDATE_ENTRY_FAIL").execute_test()

    # DELETE
    saml_provider_args = {}
    saml_provider_args['Name'] = 'S3IDP'
    saml_provider_args['SAMLMetadataDocument'] = metadata_doc_full_path
    saml_provider_args['SAMLProviderArn'] = response_elements['SAMLProviderArn']

    AuthTest("Set LDAP_SEARCH_FAIL fault point").inject_fault("LDAP_SEARCH_FAIL", "SKIP_FIRST_N_TIMES", 3).execute_test()
    test_msg = "Delete SAML provider should fail"
    AuthTest(test_msg).delete_saml_provider(**saml_provider_args).execute_test()\
            .command_response_should_have("Exception occured while deleting SAML provider")
    AuthTest("Reset LDAP_SEARCH_FAIL fault point").reset_fault("LDAP_SEARCH_FAIL").execute_test()

    AuthTest("Set LDAP_DELETE_ENTRY_FAIL fault point").inject_fault("LDAP_DELETE_ENTRY_FAIL", "FAIL_ALWAYS", 0).execute_test()
    test_msg = "Delete SAML provider should fail"
    AuthTest(test_msg).delete_saml_provider(**saml_provider_args).execute_test()\
            .command_response_should_have("Exception occured while deleting SAML provider")
    AuthTest("Reset LDAP_DELETE_ENTRY_FAIL fault point").reset_fault("LDAP_DELETE_ENTRY_FAIL").execute_test()

    test_msg = 'Delete SAML provider'
    AuthTest(test_msg).delete_saml_provider(**saml_provider_args).execute_test()\
            .command_response_should_have("SAML provider deleted.")

    test_msg = "Delete SAML provider should fail if it doesn't exist"
    AuthTest(test_msg).delete_saml_provider(**saml_provider_args).execute_test()\
            .command_response_should_have("Exception occured while deleting SAML provider")

def get_federation_token_test():
    federation_token_args = {}
    federation_token_args['Name'] = 's3root'

    AuthTest("Set LDAP_ADD_ENTRY_FAIL fault point").inject_fault("LDAP_ADD_ENTRY_FAIL", "SKIP_FIRST_N_TIMES", 1).execute_test()
    test_msg = 'Get Federation Token should fail'
    AuthTest(test_msg).get_federation_token(**federation_token_args).execute_test()\
            .command_response_should_have("Exception occured while creating federation token")
    AuthTest("Reset LDAP_ADD_ENTRY_FAIL fault point").reset_fault("LDAP_ADD_ENTRY_FAIL").execute_test()

    AuthTest("Set LDAP_ADD_ENTRY_FAIL fault point").inject_fault("LDAP_ADD_ENTRY_FAIL", "FAIL_ALWAYS", 0).execute_test()
    test_msg = 'Get Federation Token should fail'
    AuthTest(test_msg).get_federation_token(**federation_token_args).execute_test()\
            .command_response_should_have("Exception occured while creating federation token")
    AuthTest("Reset LDAP_ADD_ENTRY_FAIL fault point").reset_fault("LDAP_ADD_ENTRY_FAIL").execute_test()

    test_msg = 'Get Federation Token with invalid DurationSeconds value should fail'
    federation_token_args = {}
    federation_token_args['Name'] = 's3root'
    federation_token_args['DurationSeconds'] = '2'
    AuthTest(test_msg).get_federation_token(**federation_token_args).execute_test()\
            .command_response_should_have("Parameter validation failed")

    test_msg = 'Get Federation Token with duration of 905 seconds'
    federation_token_args = {}
    federation_token_args['Name'] = 's3root'
    federation_token_args['DurationSeconds'] = '905'
    response_pattern = "FederatedUserId = [\S]*, AccessKeyId = [\w-]*, SecretAccessKey = [\w-]*, SessionToken = [\w-]*$"
    result = AuthTest(test_msg).get_federation_token(**federation_token_args).execute_test()
    result.command_should_match_pattern(response_pattern)

before_all()
account_tests()
user_tests()
accesskey_tests()
role_tests()
saml_provider_tests()
get_federation_token_test()
