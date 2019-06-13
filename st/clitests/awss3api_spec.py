import os
from framework import Config
from framework import S3PyCliTest
from awss3api import AwsTest
from s3client_config import S3ClientConfig
from aclvalidation import AclTest

# Helps debugging
# Config.log_enabled = True
# Config.dummy_run = True
# Config.client_execution_timeout = 300 * 1000
# Config.request_timeout = 300 * 1000
# Config.socket_timeout = 300 * 1000

# Extract the upload id from response which has the following format
# [bucketname    objecctname    uploadid]

def get_upload_id(response):
    key_pairs = response.split('\t')
    return key_pairs[2]

# Run before all to setup the test environment.
print("Configuring LDAP")
S3PyCliTest('Before_all').before_all()

#******** Create Bucket ********
AwsTest('Aws can create bucket').create_bucket("seagatebucket").execute_test().command_is_successful()

#******** Put Bucket Tag ********
AwsTest('Aws can create bucket tag').put_bucket_tagging("seagatebucket", [{'Key': 'organization','Value': 'marketing'}])\
    .execute_test().command_is_successful()

#******** List Bucket Tag ********
AwsTest('Aws can list bucket tag').list_bucket_tagging("seagatebucket").execute_test().command_is_successful()\
    .command_response_should_have("organization").command_response_should_have("marketing")

#******** List bucket Tag after Updating tags ********
AwsTest('Aws can create bucket tag').put_bucket_tagging("seagatebucket", [{'Key': 'seagate','Value': 'storage'}])\
    .execute_test().command_is_successful()

AwsTest('Aws can list bucket tag').list_bucket_tagging("seagatebucket").execute_test().command_is_successful()\
    .command_response_should_not_have("organization").command_response_should_not_have("marketing")\
    .command_response_should_have("seagate").command_response_should_have("storage")

#********** Negative case to check restriction on length of bucket-tagging key length ***********
invalid_long_tag_key = 'LiXrj2I0fMJNAyhl61P7a536zhZXgkAZCNfaTLy11SkNcxmMPd7wrVf6hpHYgmw0r5KiNgt5vSFcLfmgoiQ5hizpCLotjoIWj9TYdO02NTg58XGMUX8Xol2wez42UnZb1'

AwsTest('Aws can not create bucket tag with key size greater than 128 unicode characters')\
    .put_bucket_tagging("seagatebucket", [{'Key': invalid_long_tag_key,'Value': 'marketing'}])\
    .execute_test(negative_case=True).command_should_fail().command_error_should_have("InvalidTagError")

#*********** Negative case to check restriction on length of bucket-tagging value length *********
invalid_long_tag_value = 'kJHu017I0OErlyoZLfx6fKodbEWXBBJjvCt0YuliSZvfBNPlgvecAZkUjwtG3YHRMpTDRcMMcsQAfTlrq47ZRvzsVLBAkFgBLFm9ytbfV2E6Vo6NX4uEnMaVFq9z7Mh9XzRZaCdrF7plwiMgjs1hQbOOm2VLXiVf31paJLipWw2D66DExvp0e1IqufzGQ5KzvEu5vJRAFtfiXLRhsRHWV0cvli0wVSWpK6Wg3NbSz7kMrX2MqIQ5gyZNWeT4ISzJNjLS7BVljDPh4IOSn6KAjEQLFeOZzmvUwcSU8VG92mL14EEZqzu8etBYzK499BEfM67B9bkAmiwu3Lpr6DZk6a0vBwLfBijVyOfDhGTzkXxYnqjYnGZ65S4sfnS4yeBeI0KGWAIVa39I3VwAwigL3lAmgqxrYSVJN0JgHVSapE9lb80YjVoADE6jtLAeXbv3YGfbUtagpSaj16kVdueW1wGlDmNLTuZOhKW3cSOnXLhhVKbLCff4l8CA9lh70q1Vs'

AwsTest('Aws can not create bucket tag with value size greater than 2*256 unicode characters')\
    .put_bucket_tagging("seagatebucket", [{'Key': 'organization','Value': invalid_long_tag_value}])\
    .execute_test(negative_case=True).command_should_fail().command_error_should_have("InvalidTagError")

#*********** Negative case to check restriction on length of bucket-tagging value length *********
tag_list_with_too_many_tags = []
for x in range(52):
    key = "seagate" + str(x)
    value = "storage" + str(x)
    case = {'Key': key, 'Value': value}
    tag_list_with_too_many_tags.append(case)

AwsTest('Aws can create bucket tag for more than 50 entries').put_bucket_tagging("seagatebucket", tag_list_with_too_many_tags).execute_test(negative_case=True)\
    .command_should_fail().command_error_should_have("InvalidTagError")

#************Negative case to check creation of tag for non-existant bucket*******
AwsTest('Aws can not create bucket tag on non-existant').put_bucket_tagging("seagate-bucket", [{'Key': 'seagate','Value': 'storage'}])\
    .execute_test(negative_case=True).command_should_fail().command_error_should_have("NoSuchBucket")

#************** Delete Bucket Tag ********
AwsTest('Aws can delete bucket tag').delete_bucket_tagging("seagatebucket").execute_test().command_is_successful()

AwsTest('Aws can list bucket tag').list_bucket_tagging("seagatebucket").execute_test(negative_case=True)\
    .command_should_fail().command_error_should_have("NoSuchTagSetError")

#******* delete bucket **********
AwsTest('Aws can delete bucket').delete_bucket("seagatebucket").execute_test().command_is_successful()

#******** Create Bucket ********
AwsTest('Aws can create bucket').create_bucket("seagatebuckettag").execute_test().command_is_successful()

#******** Put Object Tag ********
AwsTest('Aws can upload 3k file with tags').put_object_with_tagging("seagatebuckettag", "3kfile", 3000, "organization=marketing" ).execute_test().command_is_successful()

#******** List Object Tag ********
AwsTest('Aws can list object tag').list_object_tagging("seagatebuckettag","3kfile").execute_test().command_is_successful()\
    .command_response_should_have("organization").command_response_should_have("marketing")

#******** List Object Tag after Updating tags ********
AwsTest('Aws can update tags for 3k file').put_object_tagging("seagatebuckettag", "3kfile", [{'Key': 'seagate','Value': 'storage'}] ).execute_test().command_is_successful()

AwsTest('Aws can list bucket tag after update').list_object_tagging("seagatebuckettag","3kfile").execute_test().command_is_successful()\
    .command_response_should_not_have("organization").command_response_should_not_have("marketing")\
    .command_response_should_have("seagate").command_response_should_have("storage")

#*********** Negative case to check restriction on length of object-tagging value length *********
tag_list_with_too_many_tags = []
for x in range(11):
    key = "seagate" + str(x)
    value = "storage" + str(x)
    case = {'Key': key, 'Value': value}
    tag_list_with_too_many_tags.append(case)

AwsTest('Aws can create object tag for more than 10 entries').put_object_tagging("seagatebuckettag","3kfile", tag_list_with_too_many_tags).execute_test(negative_case=True)\
    .command_should_fail().command_error_should_have("InvalidTagError")

#************** Delete Object Tag ********
AwsTest('Aws can delete object tag').delete_object_tagging("seagatebuckettag","3kfile").execute_test().command_is_successful()

AwsTest('Aws can list object tag').list_object_tagging("seagatebuckettag","3kfile").execute_test(negative_case=True)\
    .command_should_fail().command_error_should_have("NoSuchTagSetError")

#************** Delete Object  ********
AwsTest('Aws can delete object').delete_object("seagatebuckettag","3kfile").execute_test().command_is_successful()

#************** Create a multipart upload ********
result=AwsTest('Aws can upload 10Mb file with tags').create_multipart_upload("seagatebuckettag", "10Mbfile", 10485760, "domain=storage" ).execute_test().command_is_successful()
upload_id = get_upload_id(result.status.stdout)
print(upload_id)

#************** Upload Individual parts ********
result=AwsTest('Aws can upload 5Mb first part').upload_part("seagatebuckettag", "firstpart", 5242880, "10Mbfile", "1" , upload_id).execute_test().command_is_successful()
e_tag_1 = result.status.stdout
print(e_tag_1)

result=AwsTest('Aws can upload 5Mb second part').upload_part("seagatebuckettag", "secondpart", 5242880, "10Mbfile", "2" , upload_id).execute_test().command_is_successful()
e_tag_2 = result.status.stdout
print(e_tag_2)

parts="Parts=[{ETag="+e_tag_1.strip('\n')+",PartNumber="+str(1)+"},{ETag="+e_tag_2.strip('\n')+",PartNumber="+str(2)+"}]"
print(parts)

#************** Complete multipart upload ********
result=AwsTest('Aws can complete multipart upload 10Mb file with tags').complete_multipart_upload("seagatebuckettag", "10Mbfile", parts, upload_id).execute_test().command_is_successful().command_response_should_have("seagatebuckettag/10Mbfile")

#******** List Object Tags for Multipart Upload********
AwsTest('Aws can list object tags for multipart upload').list_object_tagging("seagatebuckettag","10Mbfile").execute_test().command_is_successful()\
    .command_response_should_have("domain").command_response_should_have("storage")

#************** Delete Object  ********
AwsTest('Aws can delete object').delete_object("seagatebuckettag","10Mbfile").execute_test().command_is_successful()

#******* Delete bucket **********
AwsTest('Aws can delete bucket').delete_bucket("seagatebuckettag").execute_test().command_is_successful()


#********  Get Object ACL **********

AwsTest('Aws can create bucket').create_bucket("seagatebucketobjectacl").execute_test().command_is_successful()

AwsTest('Aws can create object').put_object("seagatebucketobjectacl", "testObject").execute_test().command_is_successful()

result=AwsTest('Aws can get object acl').get_object_acl("seagatebucketobjectacl", "testObject").execute_test().command_is_successful()

print("ACL validation started..")
AclTest('aws command has valid response').check_response_status(result)
AclTest('validate complete acl').validate_acl(result, "C12345", "s3_test", "FULL_CONTROL")
AclTest('acl has valid Owner').validate_owner(result, "C12345", "s3_test")
AclTest('acl has valid Grants').validate_grant(result, "C12345", "s3_test", 1, "FULL_CONTROL")
print("ACL validation Completed..")

AwsTest('Aws can delete object').delete_object("seagatebucketobjectacl","testObject").execute_test().command_is_successful()

#*********** Negative case to fetch acl for non-existing object ****************************
AwsTest('Aws can not fetch acl of non-existing object').get_object_acl("seagatebucketobjectacl", "testObject").execute_test(negative_case=True)\
    .command_should_fail().command_error_should_have("NoSuchKey")

AwsTest('Aws can delete bucket').delete_bucket("seagatebucketobjectacl").execute_test().command_is_successful()

#*********** Negative case to fetch acl for non-existing bucket ****************************
AwsTest('Aws can not fetch acl of non-existing bucket').get_object_acl("seagatebucketobjectacl", "testObject").execute_test(negative_case=True)\
    .command_should_fail().command_error_should_have("NoSuchBucket")
