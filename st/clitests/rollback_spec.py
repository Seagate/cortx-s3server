#!/usr/bin/python

from framework import Config
from framework import PyCliTest
from s3cmd import S3cmdTest
from s3fi import S3fiTest
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
PyCliTest('Before_all').before_all()

# Set pathstyle =false to run jclient for partial multipart upload
S3ClientConfig.pathstyle = False
S3ClientConfig.access_key_id = 'AKIAJPINPFRBTPAYOGNA'
S3ClientConfig.secret_key = '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'

# Path style tests.
Config.config_file = "pathstyle.s3cfg"

# ************ Create bucket ************
S3cmdTest('s3cmd can create bucket').create_bucket("seagatebucket").execute_test().command_is_successful()

# ************ List buckets ************
S3cmdTest('s3cmd can list buckets').list_buckets().execute_test().command_is_successful().command_response_should_have('s3://seagatebucket')

# ************ 18MB FILE Multipart Rollback TEST ***********
S3fiTest('s3cmd enable Fault injection').enable_fi("enable", "always", "clovis_idx_create_fail").execute_test().command_is_successful()
S3cmdTest('s3cmd can upload 18MB file').upload_test("seagatebucket", "18MBfile", 18000000).execute_test(True).command_should_fail()
S3cmdTest('s3cmd should not have objects after rollback').list_objects('seagatebucket').execute_test().command_is_successful().command_response_should_not_have('18MBfile')
S3fiTest('s3cmd can disable Fault injection').disable_fi("clovis_idx_create_fail").execute_test().command_is_successful()
# Set second rollback checkpoint in multipart upload
S3fiTest('s3cmd enable Fault injection').enable_fi_enablen("enable", "clovis_idx_create_fail", "2").execute_test().command_is_successful()
S3cmdTest('s3cmd can upload 18MB file').upload_test("seagatebucket", "18MBfile", 18000000).execute_test(True).command_should_fail()
S3cmdTest('s3cmd should not have objects after rollback').list_objects('seagatebucket').execute_test().command_is_successful().command_response_should_not_have('18MBfile')
S3fiTest('s3cmd can disable Fault injection').disable_fi("clovis_idx_create_fail").execute_test().command_is_successful()

# ************ Delete bucket TEST ************
S3cmdTest('s3cmd can delete bucket').delete_bucket("seagatebucket").execute_test().command_is_successful()
#


# Path style tests.
pathstyle_values = [True, False]
for i, val in enumerate(pathstyle_values):
    S3ClientConfig.pathstyle = val
    print("\nPath style = " + str(val) + "\n")

    # ************ Create bucket ************
    JClientTest('Jclient can create bucket').create_bucket("seagatebucket").execute_test().command_is_successful()

    # ************ List buckets ************
    JClientTest('Jclient can list buckets').list_buckets().execute_test().command_is_successful().command_response_should_have('seagatebucket')

    # ************ 8k FILE TEST ************
    S3fiTest('s3cmd enable Fault injection').enable_fi("enable", "always", "clovis_idx_create_fail").execute_test().command_is_successful()
    JClientTest('Jclient can upload 8k file').put_object("seagatebucket", "8kfile", 8192).execute_test(True).command_should_fail()

    JClientTest('Jclient should not have object after rollback').list_objects('seagatebucket').execute_test().command_is_successful().command_response_should_not_have('8kfile')
    S3fiTest('s3cmd can disable Fault injection').disable_fi("clovis_idx_create_fail").execute_test().command_is_successful()
    # ************ Delete bucket TEST ************
    JClientTest('Jclient can delete bucket').delete_bucket("seagatebucket").execute_test().command_is_successful()
    #
