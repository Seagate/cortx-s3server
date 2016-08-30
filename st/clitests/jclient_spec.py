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

    # ************ Create bucket ************
    JClientTest('Jclient can create bucket').create_bucket("seagatebucket").execute_test().command_is_successful()

    JClientTest('Jclient can verify bucket existence').check_bucket_exists("seagatebucket").execute_test().command_is_successful().command_response_should_have('Bucket seagatebucket exists')

    # ************ List buckets ************
    JClientTest('Jclient can list buckets').list_buckets().execute_test().command_is_successful().command_response_should_have('seagatebucket')

    # ************ 3k FILE TEST ************
    JClientTest('Jclient can verify object does not exist').head_object("seagatebucket", "3kfile").execute_test().command_is_successful().command_response_should_have('Object does not exist')

    JClientTest('Jclient can upload 3k file').put_object("seagatebucket", "3kfile", 3000).execute_test().command_is_successful()

    JClientTest('Jclient can verify object existence').head_object("seagatebucket", "3kfile").execute_test().command_is_successful().command_response_should_have("3kfile")

    JClientTest('Jclient can download 3k file').get_object("seagatebucket", "3kfile").execute_test().command_is_successful().command_created_file("3kfile")

    # ************ 8k FILE TEST ************
    JClientTest('Jclient can upload 8k file').put_object("seagatebucket", "8kfile", 8192).execute_test().command_is_successful()

    JClientTest('Jclient can download 8k file').get_object("seagatebucket", "8kfile").execute_test().command_is_successful().command_created_file("8kfile")

    # ************ OBJECT LISTING TEST ************
    JClientTest('Jclient can list objects').list_objects('seagatebucket').execute_test().command_is_successful().command_response_should_have('3kfile').command_response_should_have('8kfile')

    JClientTest('Jclient can list specific objects').list_specific_objects('seagatebucket', '3k').execute_test().command_is_successful().command_response_should_have('3kfile').command_response_should_not_have('8kfile')

    # ************ 700K FILE TEST ************
    JClientTest('Jclient can upload 700K file').put_object("seagatebucket", "700Kfile", 716800).execute_test().command_is_successful()

    JClientTest('Jclient can download 700K file').get_object("seagatebucket", "700Kfile").execute_test().command_is_successful().command_created_file("700Kfile")

    # ************ 18MB FILE TEST (Without multipart) ************
    JClientTest('Jclient can upload 18MB file').put_object("seagatebucket", "18MBfile", 18000000).execute_test().command_is_successful()

    JClientTest('Jclient can delete 18MB file').delete_object("seagatebucket", "18MBfile").execute_test().command_is_successful()

    # ************ 18MB FILE Multipart Upload TEST ***********
    JClientTest('Jclient can upload 18MB file (multipart)').put_object_multipart("seagatebucket", "18MBfile", 18000000, 15).execute_test().command_is_successful()

    JClientTest('Jclient can download 18MB file').get_object("seagatebucket", "18MBfile").execute_test().command_is_successful().command_created_file("18MBfile")

    JClientTest('Jclient can upload partial parts to test abort and list multipart.').partial_multipart_upload("seagatebucket", "18MBfile", 18000000, 1, 2).execute_test().command_is_successful()

    result = JClientTest('Jclient can list all multipart uploads.').list_multipart("seagatebucket").execute_test()
    result.command_response_should_have('18MBfile')

    upload_id = result.status.stdout.split("id - ")[1]
    print(upload_id)

    result = JClientTest('Jclient can list parts of multipart upload.').list_parts("seagatebucket", "18MBfile", upload_id).execute_test()
    result.command_response_should_have("part number - 1").command_response_should_have("part number - 2")

    JClientTest('Jclient can abort multipart upload').abort_multipart("seagatebucket", "18MBfile", upload_id).execute_test().command_is_successful()

    JClientTest('Jclient can test the multipart was aborted.').list_multipart('seagatebucket').execute_test().command_is_successful().command_response_should_not_have('18MBfile')

    # ************ DELETE OBJECT TEST ************
    JClientTest('Jclient can delete 3k file').delete_object("seagatebucket", "3kfile").execute_test().command_is_successful()

    # ************ DELETE MULTIPLE OBJECTS TEST ************
    JClientTest('Jclient can delete 8k, 700k and 18MB files and non existent 1MB file').delete_multiple_objects("seagatebucket", ["8kfile", "700Kfile", "18MBfile", "1MBfile"]).execute_test().command_is_successful()

    # ************ Delete bucket TEST ************
    JClientTest('Jclient can delete bucket').delete_bucket("seagatebucket").execute_test().command_is_successful()

    # ************ Signing algorithm test ************
    JClientTest('Jclient can create bucket seagate-bucket').create_bucket("seagate-bucket").execute_test().command_is_successful()
    JClientTest('Jclient can create bucket seagatebucket123').create_bucket("seagatebucket123").execute_test().command_is_successful()
    JClientTest('Jclient can create bucket seagate.bucket').create_bucket("seagate.bucket").execute_test().command_is_successful()
    JClientTest('Jclient can delete bucket seagate-bucket').delete_bucket("seagate-bucket").execute_test().command_is_successful()
    JClientTest('Jclient can delete bucket seagatebucket123').delete_bucket("seagatebucket123").execute_test().command_is_successful()
    JClientTest('Jclient can delete bucket seagate.bucket').delete_bucket("seagate.bucket").execute_test().command_is_successful()


# Add tests which are specific to Path style APIs

S3ClientConfig.pathstyle = True

# ************ Signing algorithm test ************
# /etc/hosts should not contains nondnsbucket. This is to test the path style APIs.
JClientTest('Jclient can create bucket nondnsbucket').create_bucket("nondnsbucket").execute_test().command_is_successful()
JClientTest('Jclient can delete bucket nondnsbucket').delete_bucket("nondnsbucket").execute_test().command_is_successful()
