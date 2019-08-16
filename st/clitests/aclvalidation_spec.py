import os
import yaml
from framework import Config
from framework import S3PyCliTest
from awss3api import AwsTest
from s3cmd import S3cmdTest
from s3client_config import S3ClientConfig
from aclvalidation import AclTest
from auth import AuthTest

# Load test config file
def load_test_config():
    conf_file = os.path.join(os.path.dirname(__file__),'s3iamcli_test_config.yaml')
    with open(conf_file, 'r') as f:
            config = yaml.safe_load(f)
            S3ClientConfig.ldapuser = config['ldapuser']
            S3ClientConfig.ldappasswd = config['ldappasswd']

# Run before all to setup the test environment.
def before_all():
    load_test_config()
    print("Configuring LDAP")
    S3PyCliTest('Before_all').before_all()

# Run before all to setup the test environment.
before_all()

#******** Create accounts************

# Create secondary account
test_msg = "Create account secaccount"
account_args = {'AccountName': 'secaccount', 'Email': 'secaccount@seagate.com', 'ldapuser': S3ClientConfig.ldapuser, 'ldappasswd': S3ClientConfig.ldappasswd}
account_response_pattern = "AccountId = [\w-]*, CanonicalId = [\w-]*, RootUserName = [\w+=,.@-]*, AccessKeyId = [\w-]*, SecretKey = [\w/+]*$"
result = AuthTest(test_msg).create_account(**account_args).execute_test()
result.command_should_match_pattern(account_response_pattern)
account_response_elements = AuthTest.get_response_elements(result.status.stdout)

secondary_access_key = account_response_elements['AccessKeyId']
secondary_secret_key = account_response_elements['SecretKey']

#********  Get ACL **********
bucket="seagatebucketobjectacl"
AwsTest('Aws can create bucket').create_bucket(bucket).execute_test().command_is_successful()

result=AwsTest('Aws can get bucket acl').get_bucket_acl(bucket).execute_test().command_is_successful()

print("Bucket ACL validation started..")
AclTest('aws command has valid response').check_response_status(result)
AclTest('validate complete acl').validate_acl(result, "C12345", "s3_test", "FULL_CONTROL")
AclTest('acl has valid Owner').validate_owner(result, "C12345", "s3_test")
AclTest('acl has valid Grants').validate_grant(result, "C12345", "s3_test", 1, "FULL_CONTROL")
print("ACL validation Completed..")

AwsTest('Aws can create object').put_object(bucket, "testObject").execute_test().command_is_successful()

result=AwsTest('Aws can get object acl').get_object_acl(bucket, "testObject").execute_test().command_is_successful()

print("Object ACL validation started..")
AclTest('aws command has valid response').check_response_status(result)
AclTest('validate complete acl').validate_acl(result, "C12345", "s3_test", "FULL_CONTROL")
AclTest('acl has valid Owner').validate_owner(result, "C12345", "s3_test")
AclTest('acl has valid Grants').validate_grant(result, "C12345", "s3_test", 1, "FULL_CONTROL")
print("ACL validation Completed..")

#*********** Negative case to fetch bucket acl for non-existing bucket ****************************
AwsTest('Aws can not fetch bucket acl of non-existing bucket').get_bucket_acl("seagateinvalidbucket").execute_test(negative_case=True)\
    .command_should_fail().command_error_should_have("NoSuchBucket")

#*********** Negative case to fetch object acl for non-existing object ****************************
AwsTest('Aws can not fetch object acl of non-existing object').get_object_acl(bucket, "testObjectInvalid").execute_test(negative_case=True)\
    .command_should_fail().command_error_should_have("NoSuchKey")

#*********** Negative case to fetch object acl for non-existing bucket ****************************
AwsTest('Aws can not fetch object acl of non-existing bucket').get_object_acl("seagateinvalidbucketobjectacl", "testObject").execute_test(negative_case=True)\
    .command_should_fail().command_error_should_have("NoSuchBucket")

AwsTest('Aws can delete object').delete_object(bucket,"testObject").execute_test().command_is_successful()

AwsTest('Aws can delete bucket').delete_bucket(bucket).execute_test().command_is_successful()

#******** Create Bucket with default account ********
AwsTest('Aws can create bucket').create_bucket("seagatebucketacl").execute_test().command_is_successful()

#*********** validate default bucket ACL *********

# Validate get-bucket-acl
'''result=AwsTest('Aws can get bucket acl').get_bucket_acl("seagatebucketacl").execute_test().command_is_successful()

print("Bucket ACL validation started..")
AclTest('aws command has valid response').check_response_status(result)
AclTest('validate complete acl').validate_acl(result, "C12345", "s3_test", "FULL_CONTROL")
AclTest('acl has valid Owner').validate_owner(result, "C12345", "s3_test")
AclTest('acl has valid Grants').validate_grant(result, "C12345", "s3_test", 1, "FULL_CONTROL")
print("ACL validation Completed..")'''

# secondary account can not get the default bucket acl
'''AwsTest('Other AWS account can not get default bucket acl').with_credentials(secondary_access_key, secondary_secret_key)\
.get_bucket_acl("seagatebucketacl", "testObject")\
.execute_test().(negative_case=True).command_should_fail().command_error_should_have("AccessDenied")'''

#********** validate get default object acl *********

# Put object in bucket - seagatebucketacl
AwsTest('Aws can create object').put_object("seagatebucketacl", "testObject").execute_test().command_is_successful()

# validate get object acl for default account
result=AwsTest('Aws can get object acl').get_object_acl("seagatebucketacl", "testObject").execute_test().command_is_successful()

print("Object ACL validation started..")
AclTest('aws command has valid response').check_response_status(result)
AclTest('validate complete acl').validate_acl(result, "C12345", "s3_test", "FULL_CONTROL")
AclTest('acl has valid Owner').validate_owner(result, "C12345", "s3_test")
AclTest('acl has valid Grants').validate_grant(result, "C12345", "s3_test", 1, "FULL_CONTROL")
print("ACL validation Completed..")

# secondary account can not get default object acl
'''AwsTest('Aws can get object acl').with_credentials(secondary_access_key, secondary_secret_key)\
.get_object_acl("seagatebucketacl", "testObject")\
.execute_test().(negative_case=True).command_should_fail().command_error_should_have("AccessDenied")'''

#********** Delete object *************

# Secondary account can not delete object from seagatebucketacl
'''AwsTest('Aws can not delete object owned by another account without permission')\
.with_credentials(secondary_access_key, secondary_secret_key)\
.delete_object("seagatebucketacl", "testObject").execute_test()\
.(negative_case=True).command_should_fail().command_error_should_have("AccessDenied")'''

# Default acount can delete object from its own bucket
AwsTest('Aws can delete object owned by itself').delete_object("seagatebucketacl","testObject").execute_test().command_is_successful()

#********** Delete bucket *************

# Secondary account can not delete bucket
'''AwsTest('AWS can not delete bucket owned by another account without permission')\
.with_credentials(secondary_access_key, secondary_secret_key)\
.delete_bucket("seagatebucketacl").execute_test().(negative_case=True)\
.command_should_fail().command_error_should_have("AccessDenied")'''

# Default account can delete bucket seagatebucketacl
AwsTest('Account owner can delete bucket').delete_bucket("seagatebucketacl").execute_test().command_is_successful()

# ********** Cleanup ****************

# delete secondary account
test_msg = "Delete account secaccount"
account_args = {'AccountName': 'secaccount', 'Email': 'secaccount@seagate.com',  'force': True}
S3ClientConfig.access_key_id = secondary_access_key
S3ClientConfig.secret_key = secondary_secret_key
AuthTest(test_msg).delete_account(**account_args).execute_test()\
            .command_response_should_have("Account deleted successfully")

#*************** Test Case 1 ***************
# TODO Enable below tests once permission header feature available
#test_msg = "Create account testAccount"
#account_args = {'AccountName': 'testAccount', 'Email': 'testAccount@seagate.com', 'ldapuser': "sgiamadmin", 'ldappasswd': "ldapadmin"}
#account_response_pattern = "AccountId = [\w-]*, CanonicalId = [\w-]*, RootUserName = [\w+=,.@-]*, AccessKeyId = [\w-]*, SecretKey = [\w/+]*$"
#result = AuthTest(test_msg).create_account(**account_args).execute_test()
#result.command_should_match_pattern(account_response_pattern)
#account_response_elements = AuthTest.get_response_elements(result.status.stdout)
#testAccount_access_key = account_response_elements['AccessKeyId']
#testAccount_secret_key = account_response_elements['SecretKey']
#testAccount_cannonicalid = account_response_elements['CanonicalId']
#testAccount_email = "testAccount@seagate.com"
#
#AwsTest('Aws can create bucket').create_bucket("putobjacltestbucket").execute_test().command_is_successful()
#cannonical_id = "id=" + testAccount_cannonicalid
#AwsTest('Aws can upload 3k file with tags').put_object_with_permission_headers("putobjacltestbucket", "3kfile", "grant-read" , cannonical_id ).execute_test().command_is_successful()
#result=AwsTest('Aws can get object acl').get_object_acl("putobjacltestbucket", "3kfile").execute_test().command_is_successful().command_response_should_have("testAccount")
#AwsTest('Aws can delete object').delete_object("putobjacltestbucket","3kfile").execute_test().command_is_successful()
#AwsTest('Aws can delete bucket').delete_bucket("putobjacltestbucket").execute_test().command_is_successful()
#
#
##***************Test Case 2 ******************
#cannonical_id = "id=" + testAccount_cannonicalid
#AwsTest('Aws can create bucket').create_bucket_with_permission_headers("authorizationtestingbucket" , "grant-write", cannonical_id).execute_test().command_is_successful()
#os.environ["AWS_ACCESS_KEY_ID"] = testAccount_access_key
#os.environ["AWS_SECRET_ACCESS_KEY"] = testAccount_secret_key
#AwsTest('Aws can upload 3k file with tags').put_object("authorizationtestingbucket", "3kfile" ).execute_test().command_is_successful()
#AwsTest('Aws can delete object').delete_object("authorizationtestingbucket","3kfile").execute_test().command_is_successful()
#del os.environ["AWS_ACCESS_KEY_ID"]
#del os.environ["AWS_SECRET_ACCESS_KEY"]
#AwsTest('Aws can delete bucket').delete_bucket("authorizationtestingbucket").execute_test().command_is_successful()
#
##**************** Test Case 3 ************
#
#AwsTest('Aws can create bucket').create_bucket("authbucket").execute_test().command_is_successful()
#cannonical_id = "id=" + testAccount_cannonicalid
#AwsTest('Aws can upload 3k file with tags').put_object_with_permission_headers("authbucket", "3kfile", "grant-write-acp" , cannonical_id ).execute_test().command_is_successful()
#os.environ["AWS_ACCESS_KEY_ID"] = testAccount_access_key
#os.environ["AWS_SECRET_ACCESS_KEY"] = testAccount_secret_key
#AwsTest('Aws can put acl').put_object_acl("authbucket", "3kfile", "grant-read" , cannonical_id ).execute_test().command_is_successful()
#del os.environ["AWS_ACCESS_KEY_ID"]
