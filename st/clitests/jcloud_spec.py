#!/usr/bin/python

import os
from framework import Config
from framework import S3PyCliTest
from jcloud import JCloudTest
from jclient import JClientTest
from s3client_config import S3ClientConfig

# Helps debugging
# Config.log_enabled = True
# Config.dummy_run = True

# Set time_readable_format to False if you want to display the time in milli seconds.
# Config.time_readable_format = False


# TODO
# DNS-compliant bucket names should not contains underscore or other special characters.
# The allowed characters are [a-zA-Z0-9.-]*
#
# Add validations to S3 server and write system tests for the same.

#  ***MAIN ENTRY POINT

# Run before all to setup the test environment.
print("Configuring LDAP")
S3PyCliTest('Before_all').before_all()

S3ClientConfig.access_key_id = 'AKIAJPINPFRBTPAYOGNA'
S3ClientConfig.secret_key = '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'

# Path style tests.
pathstyle_values = [True, False]
for i, val in enumerate(pathstyle_values):
    S3ClientConfig.pathstyle = val
    print("\nPath style = " + str(val) + "\n")

    JCloudTest('Jcloud can verify bucket does not exist').check_bucket_exists("seagatebucket").execute_test().command_is_successful().command_response_should_have('Bucket seagatebucket does not exist')

    JCloudTest('Jcloud can not get location of non existent bucket').get_bucket_location("seagatebucket").execute_test(negative_case=True).command_should_fail().command_error_should_have('The specified bucket does not exist')

    # ************ Create bucket ************
    JCloudTest('Jcloud can create bucket').create_bucket("seagatebucket").execute_test().command_is_successful()

    JCloudTest('Jcloud cannot create bucket if it exists').create_bucket("seagatebucket").execute_test(negative_case=True).command_should_fail().command_error_should_have("ResourceAlreadyExists")

    JCloudTest('Jcloud can verify bucket existence').check_bucket_exists("seagatebucket").execute_test().command_is_successful().command_response_should_have('Bucket seagatebucket exists')

    JCloudTest('Jcloud can get bucket location').get_bucket_location("seagatebucket").execute_test().command_is_successful().command_response_should_have('us-west-2')

    # ************ List buckets ************
    JCloudTest('Jcloud can list buckets').list_buckets().execute_test().command_is_successful().command_response_should_have('seagatebucket')
    JCloudTest('Jcloud can call list objects on empty bucket').list_objects('seagatebucket').execute_test().command_is_successful()

    # ************ 3k FILE TEST ************
    JCloudTest('Jcloud can verify object does not exist').head_object("seagatebucket", "test/3kfile").execute_test().command_is_successful().command_response_should_have('Object does not exist')

    JCloudTest('Jcloud can upload 3k file').put_object("seagatebucket/test/3kfile", "3kfile", 3000).execute_test().command_is_successful()

    JCloudTest('Jcloud cannot upload file to nonexistent bucket').put_object("seagate-bucket/test/3kfile", "3kfile", 3000).execute_test(negative_case=True).command_should_fail().command_error_should_have("The specified bucket does not exist")

    JCloudTest('Jcloud can verify object existence').head_object("seagatebucket", "test/3kfile").execute_test().command_is_successful().command_response_should_have('test/3kfile')

    # Current version of Jcloud does not report error in deleteContainer
    # JCloudTest('Jcloud cannot delete bucket which is not empty').delete_bucket("seagatebucket").execute_test(negative_case=True).command_should_fail().command_error_should_have("NotEmpty")

    JCloudTest('Jcloud can download 3k file').get_object("seagatebucket/test", "3kfile").execute_test().command_is_successful().command_created_file("3kfile")

    JCloudTest('Jcloud cannot download nonexistent file').get_object("seagatebucket/test", "nonexistent").execute_test(negative_case=True).command_should_fail().command_error_should_have("No such Object")

    JCloudTest('Jcloud cannot download file in nonexistent bucket').get_object("seagate-bucket/test", "nonexistent").execute_test(negative_case=True).command_should_fail().command_error_should_have("The specified bucket is not valid")

    # ************ Special Char in file name TEST ************
    JCloudTest('Jcloud can upload 3k file with special chars in filename.').put_object("seagatebucket/2016-04:32:21/3kfile", "3kfile", 3000).execute_test().command_is_successful()

    JCloudTest('Jcloud can download 3k file with special chars in filename.').get_object("seagatebucket/2016-04:32:21", "3kfile").execute_test().command_is_successful().command_created_file("3kfile")

    # ************ 8k FILE TEST ************
    JCloudTest('Jcloud can verify object does not exist').head_object("seagatebucket", "8kfile").execute_test().command_is_successful().command_response_should_have('Object does not exist')

    JCloudTest('Jcloud can upload 8k file').put_object("seagatebucket", "8kfile", 8192).execute_test().command_is_successful()

    JCloudTest('Jcloud can verify object existence').head_object("seagatebucket", "8kfile").execute_test().command_is_successful().command_response_should_have('8kfile')

    JCloudTest('Jcloud can download 8k file').get_object("seagatebucket", "8kfile").execute_test().command_is_successful().command_created_file("8kfile")

    # ************ OBJECT LISTING TEST ************
    JCloudTest('Jcloud can list objects').list_objects('seagatebucket').execute_test().command_is_successful().command_response_should_have('test').command_response_should_have('8kfile')

    JCloudTest('Jcloud cannot list objects for nonexistent bucket').list_objects('seagate-bucket').execute_test(negative_case=True).command_should_fail().command_error_should_have("The specified bucket does not exist")

    JCloudTest('Jcloud can list objects in a directory').list_specific_objects('seagatebucket', 'test').execute_test().command_is_successful().command_response_should_have('3kfile').command_response_should_not_have('8kfile')

    # ************ 700K FILE TEST ************
    JCloudTest('Jcloud can upload 700K file').put_object("seagatebucket", "700Kfile", 716800).execute_test().command_is_successful()

    JCloudTest('Jcloud can download 700K file').get_object("seagatebucket", "700Kfile").execute_test().command_is_successful().command_created_file("700Kfile")

    # ************ 18MB FILE TEST (Without multipart) ************
    JCloudTest('Jcloud can upload 18MB file').put_object("seagatebucket", "18MBfile", 18000000).execute_test().command_is_successful()

    JCloudTest('Jcloud can delete 18MB file').delete_object("seagatebucket", "18MBfile").execute_test().command_is_successful()

    # ************ 18MB FILE Multipart Upload TEST ***********
    JCloudTest('Jcloud can upload 18MB file (multipart)').put_object_multipart("seagatebucket", "18MBfile", 18000000, 15).execute_test().command_is_successful()

    JCloudTest('Jcloud can download 18MB file').get_object("seagatebucket", "18MBfile").execute_test().command_is_successful().command_created_file("18MBfile")

    JCloudTest('Jcloud cannot upload partial parts to nonexistent bucket.').partial_multipart_upload("seagate-bucket", "18MBfile", 18000000, 1, 2).execute_test(negative_case=True).command_should_fail().command_error_should_have("The specified bucket does not exist")

    JCloudTest('Jcloud can upload partial parts to test abort and list multipart.').partial_multipart_upload("seagatebucket", "18MBfile", 18000000, 1, 2).execute_test().command_is_successful()

    result = JClientTest('Jclient can list all multipart uploads.').list_multipart("seagatebucket").execute_test()
    result.command_response_should_have('18MBfile')

    upload_id = result.status.stdout.split("id - ")[1]
    print(upload_id)

    result = JClientTest('Jclient can list parts of multipart upload.').list_parts("seagatebucket", "18MBfile", upload_id).execute_test()
    result.command_response_should_have("part number - 1").command_response_should_have("part number - 2")

    # Current Jcloud version does not report error if abort failed due to invalid bucket name
    # JCloudTest('Jcloud cannot abort multipart upload on invalid bucket').abort_multipart("seagate-bucket", "18MBfile", upload_id).execute_test(negative_case=True).command_should_fail().command_error_should_have("The specified bucket does not exist")

    JCloudTest('Jcloud can abort multipart upload').abort_multipart("seagatebucket", "18MBfile", upload_id).execute_test().command_is_successful()

    JClientTest('Jclient can test the multipart was aborted.').list_multipart('seagatebucket').execute_test().command_is_successful().command_response_should_not_have('18MBfile')

    # ************ DELETE OBJECT TEST ************
    JCloudTest('Jcloud can delete 3k file').delete_object("seagatebucket", "test/3kfile").execute_test().command_is_successful()

    JCloudTest('Jcloud can delete 3k file').delete_object("seagatebucket", "2016-04:32:21/3kfile").execute_test().command_is_successful()

    # ************ DELETE MULTIPLE OBJECTS TEST ************
    JCloudTest('Jcloud can delete 8k, 700k, 18MB files and non existent 1MB file').delete_multiple_objects("seagatebucket", ["8kfile", "700Kfile", "18MBfile", "1MBfile"]).execute_test().command_is_successful()

    JCloudTest('Jcloud cannot delete multiple files when bucket does not exists').delete_multiple_objects("seagate-bucket", ["8kfile", "700Kfile", "18MBfile", "1MBfile"]).execute_test(negative_case=True).command_should_fail().command_error_should_have("The specified bucket does not exist")

    # ************ Delete bucket TEST ************
    JCloudTest('Jcloud can delete bucket').delete_bucket("seagatebucket").execute_test().command_is_successful()

    # Current version of Jcloud does not report error in deleteContainer
    # JCloudTest('Jcloud cannot delete bucket which is not empty').delete_bucket("seagatebucket").execute_test(negative_case=True).command_should_fail().command_error_should_have("NotPresent")

    # ************ Listing with prefix ************
    JCloudTest('Jclient can create bucket seagatebucket').create_bucket("seagatebucket").execute_test().command_is_successful()
    JCloudTest('Jclient can upload a/3kfile file').put_object("seagatebucket", "3kfile", 3000, prefix="a").execute_test().command_is_successful()
    JCloudTest('Jclient can upload b/3kfile file').put_object("seagatebucket", "3kfile", 3000, prefix="b").execute_test().command_is_successful()
    JCloudTest('Jclient can list specific objects with prefix a/').list_specific_objects('seagatebucket', 'a/').execute_test().command_is_successful().command_response_should_have('a/3kfile').command_response_should_not_have('b/3kfile')
    JCloudTest('Jclient can list specific objects with prefix b/').list_specific_objects('seagatebucket', 'b/').execute_test().command_is_successful().command_response_should_have('b/3kfile').command_response_should_not_have('a/3kfile')
    JCloudTest('Jclient can delete a/3kfile, b/3kfile file').delete_multiple_objects("seagatebucket", ["a/3kfile", "b/3kfile"]).execute_test().command_is_successful()
    JCloudTest('Jclient can delete bucket').delete_bucket("seagatebucket").execute_test().command_is_successful()

    # ************ Delete bucket even if parts are present(multipart) ************
    # JCloudTest('Jclient can create bucket seagatebucket').create_bucket("seagatebucket").execute_test().command_is_successful()
    # JCloudTest('Jclient can upload partial parts to test abort and list multipart.').partial_multipart_upload("seagatebucket", "18MBfile", 18000000, 1, 2).execute_test().command_is_successful()
    # JCloudTest('Jclient can delete bucket even if parts are present').delete_bucket("seagatebucket").execute_test().command_is_successful()

    # ************ Signing algorithm test ************
    JCloudTest('Jcloud can create bucket seagate-bucket').create_bucket("seagate-bucket").execute_test().command_is_successful()
    JCloudTest('Jcloud can create bucket seagatebucket123').create_bucket("seagatebucket123").execute_test().command_is_successful()
    JCloudTest('Jcloud can create bucket seagate.bucket').create_bucket("seagate.bucket").execute_test().command_is_successful()
    JCloudTest('Jcloud can delete bucket seagate-bucket').delete_bucket("seagate-bucket").execute_test().command_is_successful()
    JCloudTest('Jcloud can delete bucket seagatebucket123').delete_bucket("seagatebucket123").execute_test().command_is_successful()
    JCloudTest('Jcloud can delete bucket seagate.bucket').delete_bucket("seagate.bucket").execute_test().command_is_successful()


# Add tests which are specific to Path style APIs

S3ClientConfig.pathstyle = True

# ************ Signing algorithm test ************
# /etc/hosts should not contains nondnsbucket. This is to test the path style APIs.
JCloudTest('Jcloud can create bucket nondnsbucket').create_bucket("nondnsbucket").execute_test().command_is_successful()
JCloudTest('Jcloud can delete bucket nondnsbucket').delete_bucket("nondnsbucket").execute_test().command_is_successful()
