#!/usr/bin/python

from framework import Config
from framework import PyCliTest
from s3cmd import S3cmdTest

# Helps debugging
# Config.log_enabled = True
# Config.dummy_run = True

#  ***MAIN ENTRY POINT

# Run before all to setup the test environment.
print("Configuring LDAP")
PyCliTest('Before_all').before_all()

# Path style tests.
Config.config_file = "pathstyle.s3cfg"

# ************ Create bucket ************
S3cmdTest('s3cmd can create bucket').create_bucket("seagate_bucket").execute_test().command_is_successful()

# ************ List buckets ************
S3cmdTest('s3cmd can list buckets').list_buckets().execute_test().command_is_successful().command_response_should_have('s3://seagate_bucket')

# ************ 3k FILE TEST ************
S3cmdTest('s3cmd can upload 3k file').upload_test("seagate_bucket", "3kfile", 3000).execute_test().command_is_successful()

S3cmdTest('s3cmd can download 3k file').download_test("seagate_bucket", "3kfile").execute_test().command_is_successful().command_created_file("3kfile")

# ************ 8k FILE TEST ************
S3cmdTest('s3cmd can upload 8k file').upload_test("seagate_bucket", "8kfile", 8192).execute_test().command_is_successful()

S3cmdTest('s3cmd can download 8k file').download_test("seagate_bucket", "8kfile").execute_test().command_is_successful().command_created_file("8kfile")

# ************ OBJECT LISTING TEST ************
S3cmdTest('s3cmd can list objects').list_objects('seagate_bucket').execute_test().command_is_successful().command_response_should_have('s3://seagate_bucket/3kfile').command_response_should_have('s3://seagate_bucket/8kfile')

S3cmdTest('s3cmd can list specific objects').list_specific_objects('seagate_bucket', '3k').execute_test().command_is_successful().command_response_should_have('s3://seagate_bucket/3kfile').command_response_should_not_have('s3://seagate_bucket/8kfile')

# ************ DELETE OBJECT TEST ************
S3cmdTest('s3cmd can delete 3k file').delete_test("seagate_bucket", "3kfile").execute_test().command_is_successful()

S3cmdTest('s3cmd can delete 8k file').delete_test("seagate_bucket", "8kfile").execute_test().command_is_successful()

# ************ 700K FILE TEST ************
S3cmdTest('s3cmd can upload 700K file').upload_test("seagate_bucket", "700Kfile", 716800).execute_test().command_is_successful()

S3cmdTest('s3cmd can download 700K file').download_test("seagate_bucket", "700Kfile").execute_test().command_is_successful().command_created_file("700Kfile")

S3cmdTest('s3cmd can delete 700K file').delete_test("seagate_bucket", "700Kfile").execute_test().command_is_successful()

# ************ Delete bucket TEST ************
S3cmdTest('s3cmd can delete bucket').delete_bucket("seagate_bucket").execute_test().command_is_successful()

# Virtual Host style tests.
Config.config_file = "virtualhoststyle.s3cfg"

# ************ Create bucket ************
S3cmdTest('s3cmd can create bucket').create_bucket("seagate_bucket").execute_test().command_is_successful()

# ************ List buckets ************
S3cmdTest('s3cmd can list buckets').list_buckets().execute_test().command_is_successful().command_response_should_have('s3://seagate_bucket')

# ************ 3k FILE TEST ************
S3cmdTest('s3cmd can upload 3k file').upload_test("seagate_bucket", "3kfile", 3000).execute_test().command_is_successful()

S3cmdTest('s3cmd can download 3k file').download_test("seagate_bucket", "3kfile").execute_test().command_is_successful().command_created_file("3kfile")

# ************ 8k FILE TEST ************
S3cmdTest('s3cmd can upload 8k file').upload_test("seagate_bucket", "8kfile", 8192).execute_test().command_is_successful()

S3cmdTest('s3cmd can download 8k file').download_test("seagate_bucket", "8kfile").execute_test().command_is_successful().command_created_file("8kfile")

# ************ OBJECT LISTING TEST ************
S3cmdTest('s3cmd can list objects').list_objects('seagate_bucket').execute_test().command_is_successful().command_response_should_have('s3://seagate_bucket/3kfile').command_response_should_have('s3://seagate_bucket/8kfile')

S3cmdTest('s3cmd can list specific objects').list_specific_objects('seagate_bucket', '3k').execute_test().command_is_successful().command_response_should_have('s3://seagate_bucket/3kfile').command_response_should_not_have('s3://seagate_bucket/8kfile')

# ************ DELETE OBJECT TEST ************
S3cmdTest('s3cmd can delete 3k file').delete_test("seagate_bucket", "3kfile").execute_test().command_is_successful()

S3cmdTest('s3cmd can delete 8k file').delete_test("seagate_bucket", "8kfile").execute_test().command_is_successful()

# ************ 700K FILE TEST ************
S3cmdTest('s3cmd can upload 700K file').upload_test("seagate_bucket", "700Kfile", 716800).execute_test().command_is_successful()

S3cmdTest('s3cmd can download 700K file').download_test("seagate_bucket", "700Kfile").execute_test().command_is_successful().command_created_file("700Kfile")

S3cmdTest('s3cmd can delete 700K file').delete_test("seagate_bucket", "700Kfile").execute_test().command_is_successful()

# ************ Delete bucket TEST ************
S3cmdTest('s3cmd can delete bucket').delete_bucket("seagate_bucket").execute_test().command_is_successful()
