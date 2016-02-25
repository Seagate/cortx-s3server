#!/usr/bin/python

from framework import Config
from framework import PyCliTest
from s3cmd import S3cmdTest

# Helps debugging
# Config.log_enabled = True
# Config.dummy_run = True

# TODO
# DNS-compliant bucket names should not contains underscore or other special characters.
# The allowed characters are [a-zA-Z0-9.-]*
#
# Add validations to S3 server and write system tests for the same.

#  ***MAIN ENTRY POINT

# Run before all to setup the test environment.
print("Configuring LDAP")
PyCliTest('Before_all').before_all()

# Path style tests.
Config.config_file = "pathstyle.s3cfg"

# ************ Create bucket ************
S3cmdTest('s3cmd can create bucket').create_bucket("seagatebucket").execute_test().command_is_successful()

# ************ List buckets ************
S3cmdTest('s3cmd can list buckets').list_buckets().execute_test().command_is_successful().command_response_should_have('s3://seagatebucket')

# ************ 3k FILE TEST ************
S3cmdTest('s3cmd can upload 3k file').upload_test("seagatebucket", "3kfile", 3000).execute_test().command_is_successful()

S3cmdTest('s3cmd can download 3k file').download_test("seagatebucket", "3kfile").execute_test().command_is_successful().command_created_file("3kfile")

# ************ 8k FILE TEST ************
S3cmdTest('s3cmd can upload 8k file').upload_test("seagatebucket", "8kfile", 8192).execute_test().command_is_successful()

S3cmdTest('s3cmd can download 8k file').download_test("seagatebucket", "8kfile").execute_test().command_is_successful().command_created_file("8kfile")

# ************ OBJECT LISTING TEST ************
S3cmdTest('s3cmd can list objects').list_objects('seagatebucket').execute_test().command_is_successful().command_response_should_have('s3://seagatebucket/3kfile').command_response_should_have('s3://seagatebucket/8kfile')

S3cmdTest('s3cmd can list specific objects').list_specific_objects('seagatebucket', '3k').execute_test().command_is_successful().command_response_should_have('s3://seagatebucket/3kfile').command_response_should_not_have('s3://seagatebucket/8kfile')

# ************ DELETE OBJECT TEST ************
S3cmdTest('s3cmd can delete 3k file').delete_test("seagatebucket", "3kfile").execute_test().command_is_successful()

S3cmdTest('s3cmd can delete 8k file').delete_test("seagatebucket", "8kfile").execute_test().command_is_successful()

# ************ 700K FILE TEST ************
S3cmdTest('s3cmd can upload 700K file').upload_test("seagatebucket", "700Kfile", 716800).execute_test().command_is_successful()

S3cmdTest('s3cmd can download 700K file').download_test("seagatebucket", "700Kfile").execute_test().command_is_successful().command_created_file("700Kfile")

S3cmdTest('s3cmd can delete 700K file').delete_test("seagatebucket", "700Kfile").execute_test().command_is_successful()

# ************ Delete bucket TEST ************
S3cmdTest('s3cmd can delete bucket').delete_bucket("seagatebucket").execute_test().command_is_successful()

# ************ Signing algorithm test ************
S3cmdTest('s3cmd can create bucket seagate-bucket').create_bucket("seagate-bucket").execute_test().command_is_successful()
S3cmdTest('s3cmd can create bucket SEAGATEBUCKET123').create_bucket("SEAGATEBUCKET123").execute_test().command_is_successful()
S3cmdTest('s3cmd can delete bucket seagate-bucket').delete_bucket("seagate-bucket").execute_test().command_is_successful()
S3cmdTest('s3cmd can delete bucket SEAGATEBUCKET123').delete_bucket("SEAGATEBUCKET123").execute_test().command_is_successful()

# Virtual Host style tests.
Config.config_file = "virtualhoststyle.s3cfg"

# ************ Create bucket ************
S3cmdTest('s3cmd can create bucket').create_bucket("seagatebucket").execute_test().command_is_successful()

# ************ List buckets ************
S3cmdTest('s3cmd can list buckets').list_buckets().execute_test().command_is_successful().command_response_should_have('s3://seagatebucket')

# ************ 3k FILE TEST ************
S3cmdTest('s3cmd can upload 3k file').upload_test("seagatebucket", "3kfile", 3000).execute_test().command_is_successful()

S3cmdTest('s3cmd can download 3k file').download_test("seagatebucket", "3kfile").execute_test().command_is_successful().command_created_file("3kfile")

# ************ 8k FILE TEST ************
S3cmdTest('s3cmd can upload 8k file').upload_test("seagatebucket", "8kfile", 8192).execute_test().command_is_successful()

S3cmdTest('s3cmd can download 8k file').download_test("seagatebucket", "8kfile").execute_test().command_is_successful().command_created_file("8kfile")

# ************ OBJECT LISTING TEST ************
S3cmdTest('s3cmd can list objects').list_objects('seagatebucket').execute_test().command_is_successful().command_response_should_have('s3://seagatebucket/3kfile').command_response_should_have('s3://seagatebucket/8kfile')

S3cmdTest('s3cmd can list specific objects').list_specific_objects('seagatebucket', '3k').execute_test().command_is_successful().command_response_should_have('s3://seagatebucket/3kfile').command_response_should_not_have('s3://seagatebucket/8kfile')

# ************ DELETE OBJECT TEST ************
S3cmdTest('s3cmd can delete 3k file').delete_test("seagatebucket", "3kfile").execute_test().command_is_successful()

S3cmdTest('s3cmd can delete 8k file').delete_test("seagatebucket", "8kfile").execute_test().command_is_successful()

# ************ 700K FILE TEST ************
S3cmdTest('s3cmd can upload 700K file').upload_test("seagatebucket", "700Kfile", 716800).execute_test().command_is_successful()

S3cmdTest('s3cmd can download 700K file').download_test("seagatebucket", "700Kfile").execute_test().command_is_successful().command_created_file("700Kfile")

S3cmdTest('s3cmd can delete 700K file').delete_test("seagatebucket", "700Kfile").execute_test().command_is_successful()

# ************ Delete bucket TEST ************
S3cmdTest('s3cmd can delete bucket').delete_bucket("seagatebucket").execute_test().command_is_successful()

# ************ Signing algorithm test ************
S3cmdTest('s3cmd can create bucket seagate-bucket').create_bucket("seagate-bucket").execute_test().command_is_successful()
S3cmdTest('s3cmd can create bucket SEAGATEBUCKET123').create_bucket("SEAGATEBUCKET123").execute_test().command_is_successful()
S3cmdTest('s3cmd can delete bucket seagate-bucket').delete_bucket("seagate-bucket").execute_test().command_is_successful()
S3cmdTest('s3cmd can delete bucket SEAGATEBUCKET123').delete_bucket("SEAGATEBUCKET123").execute_test().command_is_successful()
