#!/usr/bin/python

import os
from framework import Config
from framework import S3PyCliTest
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

    JClientTest('Jclient can verify bucket does not exist').check_bucket_exists("seagatebucket").execute_test().command_is_successful().command_response_should_have('Bucket seagatebucket does not exist')

    JClientTest('Jclient cannot get bucket location for nonexistent bucket').get_bucket_location("seagatebucket").execute_test(negative_case=True).command_should_fail().command_error_should_have('No such bucket')

    # get-bucket-acl: no bucket exists
    JClientTest('Jclient can (not) get bucket ACL').get_acl("seagatebucket").execute_test(negative_case=True).command_should_fail().command_error_should_have('No such bucket')

    # ************ Create bucket ************
    JClientTest('Jclient can create bucket').create_bucket("seagatebucket", "us-west-2").execute_test().command_is_successful()

    JClientTest('Jclient cannot create bucket if it exists').create_bucket("seagatebucket", "us-west-2").execute_test(negative_case=True).command_should_fail().command_error_should_have("BucketAlreadyExists")

    JClientTest('Jclient can get bucket location').get_bucket_location("seagatebucket").execute_test().command_is_successful().command_response_should_have('us-west-2')

    JClientTest('Jclient can verify bucket existence').check_bucket_exists("seagatebucket").execute_test().command_is_successful().command_response_should_have('Bucket seagatebucket exists')

    # ************ List buckets ************
    JClientTest('Jclient can list buckets').list_buckets().execute_test().command_is_successful().command_response_should_have('seagatebucket')
    JClientTest('Jclient can call list objects on empty bucket').list_objects('seagatebucket').execute_test().command_is_successful()

    # get-bucket-acl
    JClientTest('Jclient can get bucket ACL').get_acl("seagatebucket").execute_test().command_is_successful().command_response_should_have('tester: FULL_CONTROL')

    # ************ 3k FILE TEST ************
    JClientTest('Jclient can verify object does not exist').head_object("seagatebucket", "3kfile").execute_test(negative_case=True).command_should_fail().command_error_should_have('Bucket or Object does not exist')

    JClientTest('Jclient cannot verify object in nonexistent bucket').head_object("seagate-bucket", "3kfile").execute_test(negative_case=True).command_should_fail().command_error_should_have("Bucket or Object does not exist")

    JClientTest('Jclient can (not) get object acl').get_acl("seagatebucket", "3kfile").execute_test(negative_case=True).command_should_fail().command_error_should_have('No such object')

    JClientTest('Jclient can upload 3k file').put_object("seagatebucket", "3kfile", 3000).execute_test().command_is_successful()

    JClientTest('Jclient can upload 3k file in chunked mode').put_object("seagatebucket", "3kfilec", 3000, chunked=True).execute_test().command_is_successful()

    JClientTest('Jclient cannot upload file to nonexistent bucket').put_object("seagate-bucket", "3kfile", 3000).execute_test(negative_case=True).command_should_fail().command_error_should_have("The specified bucket does not exist.")

    JClientTest('Jclient cannot upload chunked file to nonexistent bucket').put_object("seagate-bucket", "3kfilec", 3000, chunked=True).execute_test(negative_case=True).command_should_fail().command_error_should_have("The specified bucket does not exist.")

    JClientTest('Jclient cannot delete bucket which is not empty').delete_bucket("seagatebucket").execute_test(negative_case=True).command_should_fail().command_error_should_have("BucketNotEmpty")

    JClientTest('Jclient can verify object existence').head_object("seagatebucket", "3kfile").execute_test().command_is_successful().command_response_should_have("3kfile")

    JClientTest('Jclient can get object acl').get_acl("seagatebucket", "3kfile").execute_test().command_is_successful().command_response_should_have('tester: FULL_CONTROL')

    # ACL Tests.
    # Bucket ACL Tests.
    JClientTest('Jclient can set public ACL on bucket').set_acl("seagatebucket", action="acl-public")\
        .execute_test().command_is_successful().command_response_should_have("ACL set to Public")

    JClientTest('Jclient cannot set ACL on nonexistent bucket').set_acl("seagate-bucket", action="acl-public")\
        .execute_test(negative_case=True).command_should_fail()\
        .command_error_should_have("The specified bucket does not exist")

    JClientTest('Jclient can verify public ACL on bucket').get_acl("seagatebucket")\
        .execute_test().command_is_successful().command_response_should_have("*anon*: READ")

    JClientTest('Jclient cannot verify ACL on nonexistent bucket').get_acl("seagate-bucket")\
        .execute_test(negative_case=True).command_should_fail()\
        .command_error_should_have("No such bucket")

    JClientTest('Jclient can set private ACL on bucket').set_acl("seagatebucket", action="acl-private")\
        .execute_test().command_is_successful().command_response_should_have("ACL set to Private")

    JClientTest('Jclient can verify private ACL on bucket').get_acl("seagatebucket")\
        .execute_test().command_is_successful().command_response_should_not_have("*anon*: READ")

    JClientTest('Jclient can grant READ permission on bucket').set_acl("seagatebucket",
        action="acl-grant", permission="READ:123:tester")\
        .execute_test().command_is_successful().command_response_should_have("Grant ACL successful")

    JClientTest('Jclient can verify READ ACL on bucket').get_acl("seagatebucket")\
        .execute_test().command_is_successful().command_response_should_have("tester: READ")\
        .command_response_should_not_have("WRITE")

    JClientTest('Jclient can grant WRITE permission on bucket').set_acl("seagatebucket",
        action="acl-grant", permission="WRITE:123")\
        .execute_test().command_is_successful().command_response_should_have("Grant ACL successful")

    JClientTest('Jclient can verify WRITE ACL on bucket').get_acl("seagatebucket")\
        .execute_test().command_is_successful().command_response_should_have("tester: READ")\
        .command_response_should_have("tester: WRITE")

    JClientTest('Jclient can revoke WRITE permission on bucket').set_acl("seagatebucket",
        action="acl-revoke", permission="WRITE:123")\
        .execute_test().command_is_successful().command_response_should_have("Revoke ACL successful")

    JClientTest('Jclient can verify WRITE ACL is revoked on bucket').get_acl("seagatebucket")\
        .execute_test().command_is_successful().command_response_should_not_have("WRITE")

    # Object ACL Tests.
    JClientTest('Jclient can set public ACL on object').set_acl("seagatebucket", "3kfile",
        action="acl-public")\
        .execute_test().command_is_successful().command_response_should_have("ACL set to Public")

    JClientTest('Jclient cannot set ACL on nonexistent object').set_acl("seagatebucket", "abc",
        action="acl-public")\
        .execute_test(negative_case=True).command_should_fail()\
        .command_error_should_have("The specified key does not exist")

    JClientTest('Jclient can verify public ACL on object').get_acl("seagatebucket", "3kfile")\
        .execute_test().command_is_successful().command_response_should_have("*anon*: READ")

    JClientTest('Jclient cannot verify ACL on nonexistent object').get_acl("seagatebucket", "abc")\
        .execute_test(negative_case=True).command_should_fail()\
        .command_error_should_have("No such object")

    JClientTest('Jclient can set private ACL on object').set_acl("seagatebucket", "3kfile",
        action="acl-private")\
        .execute_test().command_is_successful().command_response_should_have("ACL set to Private")

    JClientTest('Jclient can verify private ACL on object').get_acl("seagatebucket", "3kfile")\
        .execute_test().command_is_successful().command_response_should_not_have("*anon*: READ")

    JClientTest('Jclient can grant READ permission on object').set_acl("seagatebucket", "3kfile",
        action="acl-grant", permission="READ:123:tester")\
        .execute_test().command_is_successful().command_response_should_have("Grant ACL successful")

    JClientTest('Jclient can verify READ ACL on object').get_acl("seagatebucket", "3kfile")\
        .execute_test().command_is_successful().command_response_should_have("tester: READ")\
        .command_response_should_not_have("WRITE")

    JClientTest('Jclient can grant WRITE permission on object').set_acl("seagatebucket", "3kfile",
        action="acl-grant", permission="WRITE:123")\
        .execute_test().command_is_successful().command_response_should_have("Grant ACL successful")

    JClientTest('Jclient can verify WRITE ACL on object').get_acl("seagatebucket", "3kfile")\
        .execute_test().command_is_successful().command_response_should_have("tester: READ")\
        .command_response_should_have("tester: WRITE")

    JClientTest('Jclient can revoke WRITE permission on object').set_acl("seagatebucket", "3kfile",
        action="acl-revoke", permission="WRITE:123")\
        .execute_test().command_is_successful().command_response_should_have("Revoke ACL successful")

    JClientTest('Jclient can verify WRITE ACL is revoked on object').get_acl("seagatebucket", "3kfile")\
        .execute_test().command_is_successful().command_response_should_not_have("WRITE")

    JClientTest('Jclient can download 3k file').get_object("seagatebucket", "3kfile").execute_test().command_is_successful().command_created_file("3kfile")

    JClientTest('Jclient can download 3k file uploaded in chunked mode').get_object("seagatebucket", "3kfilec").execute_test().command_is_successful().command_created_file("3kfilec")

    JClientTest('Jclient cannot download nonexistent file').get_object("seagatebucket", "nonexistent").execute_test(negative_case=True).command_should_fail().command_error_should_have("The specified key does not exist")

    JClientTest('Jclient cannot download file in nonexistent bucket').get_object("seagate-bucket", "nonexistent").execute_test(negative_case=True).command_should_fail().command_error_should_have("The specified bucket is not valid")

    # ************ 8k FILE TEST ************
    JClientTest('Jclient can upload 8k file').put_object("seagatebucket", "8kfile", 8192).execute_test().command_is_successful()

    JClientTest('Jclient can upload 8k file in chunked mode').put_object("seagatebucket", "8kfilec", 8192).execute_test().command_is_successful()

    JClientTest('Jclient can download 8k file').get_object("seagatebucket", "8kfile").execute_test().command_is_successful().command_created_file("8kfile")

    JClientTest('Jclient can download 8k file uploaded in chunked mode').get_object("seagatebucket", "8kfilec").execute_test().command_is_successful().command_created_file("8kfilec")

    # ************ OBJECT LISTING TEST ************
    JClientTest('Jclient can list objects').list_objects('seagatebucket').execute_test().command_is_successful().command_response_should_have('3kfile').command_response_should_have('8kfile')

    JClientTest('Jclient cannot list objects for nonexistent bucket').list_objects('seagate-bucket').execute_test(negative_case=True).command_should_fail().command_error_should_have("The specified bucket does not exist")

    JClientTest('Jclient can list specific objects').list_specific_objects('seagatebucket', '3k').execute_test().command_is_successful().command_response_should_have('3kfile').command_response_should_not_have('8kfile')

    # ************ 700K FILE TEST ************
    JClientTest('Jclient can upload 700K file').put_object("seagatebucket", "700Kfile", 716800).execute_test().command_is_successful()

    JClientTest('Jclient can upload 700K file in chunked mode').put_object("seagatebucket", "700Kfilec", 716800, chunked=True).execute_test().command_is_successful()

    JClientTest('Jclient can download 700K file').get_object("seagatebucket", "700Kfile").execute_test().command_is_successful().command_created_file("700Kfile")

    JClientTest('Jclient can download 700K file uploaded in chunked mode').get_object("seagatebucket", "700Kfilec").execute_test().command_is_successful().command_created_file("700Kfilec")

    # ************ 18MB FILE TEST (Without multipart) ************
    JClientTest('Jclient can upload 18MB file').put_object("seagatebucket", "18MBfile", 18000000).execute_test().command_is_successful()

    JClientTest('Jclient can delete 18MB file').delete_object("seagatebucket", "18MBfile").execute_test().command_is_successful()

    JClientTest('Jclient should not have object after its delete').list_objects('seagatebucket').execute_test().command_is_successful().command_response_should_not_have('18MBfile')

    # ************ 18MB FILE Multipart Upload TEST ***********
    JClientTest('Jclient can upload 18MB file (multipart)').put_object_multipart("seagatebucket", "18MBfile", 18000000, 15).execute_test().command_is_successful()

    JClientTest('Jclient cannot upload 18MB file (multipart) to nonexistent bucket').put_object_multipart("seagate-bucket", "18MBfile", 18000000, 15).execute_test(negative_case=True).command_should_fail().command_error_should_have("The specified bucket does not exist")

    JClientTest('Jclient can download 18MB file').get_object("seagatebucket", "18MBfile").execute_test().command_is_successful().command_created_file("18MBfile")

    JClientTest('Jclient cannot upload 18MB file (multipart) in chunked mode to nonexistent bucket').put_object_multipart("seagate-bucket", "18MBfilec", 18000000, 15, chunked=True).execute_test(negative_case=True).command_should_fail().command_error_should_have("The specified bucket does not exist")

    JClientTest('Jclient can upload 18MB file (multipart) in chunked mode').put_object_multipart("seagatebucket", "18MBfilec", 18000000, 15, chunked=True).execute_test().command_is_successful()

    JClientTest('Jclient can download 18MB file uploaded in chunked mode').get_object("seagatebucket", "18MBfilec").execute_test().command_is_successful().command_created_file("18MBfilec")

    JClientTest('Jclient cannot upload partial parts to nonexistent bucket.').partial_multipart_upload("seagate-bucket", "18MBfile", 18000000, 1, 2).execute_test(negative_case=True).command_should_fail().command_error_should_have("The specified bucket does not exist")

    JClientTest('Jclient can upload partial parts to test abort and list multipart.').partial_multipart_upload("seagatebucket", "18MBfile", 18000000, 1, 2).execute_test().command_is_successful()

    JClientTest('Jclient cannot list all multipart uploads on nonexistent bucket.').list_multipart("seagate-bucket").execute_test(negative_case=True).command_should_fail().command_error_should_have("The specified bucket does not exist")

    result = JClientTest('Jclient can list all multipart uploads.').list_multipart("seagatebucket").execute_test()
    result.command_response_should_have('18MBfile')

    upload_id = result.status.stdout.split("id - ")[1]
    print(upload_id)

    JClientTest('Jclient cannot list parts of multipart upload on invalid bucket.').list_parts("seagate-bucket", "18MBfile", upload_id).execute_test(negative_case=True).command_should_fail().command_error_should_have("The specified bucket does not exist")

    JClientTest('Jclient cannot abort multipart upload on invalid bucket.').abort_multipart("seagate-bucket", "18MBfile", upload_id).execute_test(negative_case=True).command_should_fail().command_error_should_have("The specified bucket does not exist")

    result = JClientTest('Jclient can list parts of multipart upload.').list_parts("seagatebucket", "18MBfile", upload_id).execute_test()
    result.command_response_should_have("part number - 1").command_response_should_have("part number - 2")

    JClientTest('Jclient cannot abort multipart upload on invalid bucket').abort_multipart("seagate-bucket", "18MBfile", upload_id).execute_test(negative_case=True).command_should_fail().command_error_should_have("The specified bucket does not exist")

    JClientTest('Jclient can abort multipart upload').abort_multipart("seagatebucket", "18MBfile", upload_id).execute_test().command_is_successful()

    JClientTest('Jclient can test the multipart was aborted.').list_multipart('seagatebucket').execute_test().command_is_successful().command_response_should_not_have('18MBfile')

    # ************ DELETE OBJECT TEST ************
    JClientTest('Jclient can delete 3k file').delete_object("seagatebucket", "3kfile").execute_test().command_is_successful()

    JClientTest('Jclient should not have object after its deletion').list_objects('seagatebucket').execute_test().command_is_successful().command_response_should_not_have('3kfile ')

    JClientTest('Jclient cannot delete file in nonexistent bucket').delete_object("seagate-bucket", "3kfile").execute_test(negative_case=True).command_should_fail().command_error_should_have("The specified bucket does not exist")

    JClientTest('Jclient can delete nonexistent file').delete_object("seagatebucket", "3kfile").execute_test().command_is_successful()

    # ************ DELETE MULTIPLE OBJECTS TEST ************
    JClientTest('Jclient can delete 8k, 700k and 18MB files and non existent 1MB file').delete_multiple_objects("seagatebucket", ["8kfile", "700Kfile", "18MBfile", "1MBfile", "3kfilec", "8kfilec", "700Kfilec", "18MBfilec"]).execute_test().command_is_successful()

    JClientTest('Jclient should not list deleted objects').list_objects('seagatebucket').execute_test().command_is_successful().command_response_should_not_have('8kfile').command_response_should_not_have('700Kfile').command_response_should_not_have('18MBfile').command_response_should_not_have('3kfilec').command_response_should_not_have('8kfilec').command_response_should_not_have('700Kfilec').command_response_should_not_have('18MBfilec')

    JClientTest('Jclient multiple delete should succeed when objects not present').delete_multiple_objects("seagatebucket", ["8kfile", "700Kfile", "18MBfile"]).execute_test().command_is_successful()

    JClientTest('Jclient cannot delete multiple files when bucket does not exists').delete_multiple_objects("seagate-bucket", ["8kfile", "700Kfile", "18MBfile", "1MBfile"]).execute_test(negative_case=True).command_should_fail().command_error_should_have("The specified bucket does not exist")

    # ************ Delete bucket TEST ************
    JClientTest('Jclient can delete bucket').delete_bucket("seagatebucket").execute_test().command_is_successful()

    JClientTest('Jclient should not have bucket after its deletion').list_buckets().execute_test().command_is_successful().command_response_should_not_have('seagatebucket')

    JClientTest('Jclient cannot delete nonexistent bucket').delete_bucket("seagatebucket").execute_test(negative_case=True).command_should_fail().command_error_should_have("The specified bucket does not exist")

    # ************ Listing with prefix ************
    JClientTest('Jclient can create bucket seagatebucket').create_bucket("seagatebucket").execute_test().command_is_successful()
    JClientTest('Jclient can upload a/3kfile file').put_object("seagatebucket", "3kfile", 3000, prefix="a").execute_test().command_is_successful()
    JClientTest('Jclient can upload b/3kfile file').put_object("seagatebucket", "3kfile", 3000, prefix="b").execute_test().command_is_successful()
    JClientTest('Jclient can list specific objects with prefix a/').list_specific_objects('seagatebucket', 'a/').execute_test().command_is_successful().command_response_should_have('a/3kfile').command_response_should_not_have('b/3kfile')
    JClientTest('Jclient can list specific objects with prefix b/').list_specific_objects('seagatebucket', 'b/').execute_test().command_is_successful().command_response_should_have('b/3kfile').command_response_should_not_have('a/3kfile')
    JClientTest('Jclient can delete a/3kfile, b/3kfile file').delete_multiple_objects("seagatebucket", ["a/3kfile", "b/3kfile"]).execute_test().command_is_successful()
    JClientTest('Jclient can delete bucket').delete_bucket("seagatebucket").execute_test().command_is_successful()

    # ************ Delete bucket even if parts are present(multipart) ************
    JClientTest('Jclient can create bucket seagatebucket').create_bucket("seagatebucket").execute_test().command_is_successful()
    JClientTest('Jclient can upload partial parts to test abort and list multipart.').partial_multipart_upload("seagatebucket", "18MBfile", 18000000, 1, 2).execute_test().command_is_successful()
    JClientTest('Jclient can delete bucket even if parts are present').delete_bucket("seagatebucket").execute_test().command_is_successful()

    # ************ Signing algorithm test ************
    JClientTest('Jclient can create bucket seagate-bucket').create_bucket("seagate-bucket").execute_test().command_is_successful()
    JClientTest('Jclient can create bucket seagatebucket123').create_bucket("seagatebucket123").execute_test().command_is_successful()
    JClientTest('Jclient can create bucket seagate.bucket').create_bucket("seagate.bucket").execute_test().command_is_successful()
    JClientTest('Jclient can delete bucket seagate-bucket').delete_bucket("seagate-bucket").execute_test().command_is_successful()
    JClientTest('Jclient can delete bucket seagatebucket123').delete_bucket("seagatebucket123").execute_test().command_is_successful()
    JClientTest('Jclient can delete bucket seagate.bucket').delete_bucket("seagate.bucket").execute_test().command_is_successful()
    JClientTest('Jclient should not list bucket after its deletion').list_buckets().execute_test().command_is_successful().command_response_should_not_have('seagatebucket').command_response_should_not_have('seagatebucket123').command_response_should_not_have('seagate.bucket').command_response_should_not_have('seagate-bucket')


# Add tests which are specific to Path style APIs

S3ClientConfig.pathstyle = True

# ************ Signing algorithm test ************
# /etc/hosts should not contains nondnsbucket. This is to test the path style APIs.
JClientTest('Jclient can create bucket nondnsbucket').create_bucket("nondnsbucket").execute_test().command_is_successful()
JClientTest('Jclient can delete bucket nondnsbucket').delete_bucket("nondnsbucket").execute_test().command_is_successful()
