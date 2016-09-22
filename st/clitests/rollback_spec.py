#!/usr/bin/python

from framework import Config
from framework import S3PyCliTest
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
S3PyCliTest('Before_all').before_all()

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

# ************  OBJ create FI ***************
S3fiTest('s3cmd enable FI Obj create').enable_fi("enable", "always", "clovis_obj_create_fail").execute_test().command_is_successful()
S3cmdTest('s3cmd can not upload 3k file').upload_test("seagatebucket", "3kfile", 3000).execute_test(True).command_should_fail()
S3cmdTest('s3cmd can not upload 18MB file').upload_test("seagatebucket", "18MBfile", 18000000).execute_test(True).command_should_fail()
S3fiTest('s3cmd disable Fault injection').disable_fi("clovis_obj_create_fail").execute_test().command_is_successful()

#*************  PUT KV FI ***************
S3fiTest('s3cmd enable FI PUT KV').enable_fi("enable", "always", "clovis_kv_put_fail").execute_test().command_is_successful()
S3cmdTest('s3cmd can not upload 3k file').upload_test("seagatebucket", "3kfile", 3000).execute_test(True).command_should_fail()
S3cmdTest('s3cmd can not upload 18MB file').upload_test("seagatebucket", "18MBfile", 18000000).execute_test(True).command_should_fail()
S3fiTest('s3cmd disable Fault injection').disable_fi("clovis_kv_put_fail").execute_test().command_is_successful()

#************** upload objects *************
S3cmdTest('s3cmd upload 3k file').upload_test("seagatebucket", "3kfile", 3000).execute_test().command_is_successful()
S3cmdTest('s3cmd upload 18MB file').upload_test("seagatebucket", "18MBfile", 18000000).execute_test().command_is_successful()

# **************** OBJ DELETE FI  ****************
S3fiTest('s3cmd enable FI OBJ Delete').enable_fi("enable", "always", "clovis_obj_delete_fail").execute_test().command_is_successful()
S3cmdTest('s3cmd can not delete 3k file').delete_test("seagatebucket", "3kfile").execute_test(True).command_should_fail()
S3cmdTest('s3cmd can not delete 18MB file').delete_test("seagatebucket", "18MBfile").execute_test(True).command_should_fail()
S3fiTest('s3cmd disable Fault injection').disable_fi("clovis_obj_delete_fail").execute_test().command_is_successful()

#**************** GET KV FI  ****************
S3fiTest('s3cmd enable FI GET KV').enable_fi("enable", "always", "clovis_kv_get_fail").execute_test().command_is_successful()
S3cmdTest('s3cmd can not download 3k file').download_test("seagatebucket", "3kfile").execute_test(True).command_should_fail()
S3cmdTest('s3cmd can not download 18MB file').download_test("seagatebucket", "18MBfile").execute_test(True).command_should_fail()
S3fiTest('s3cmd disable Fault injection').disable_fi("clovis_kv_get_fail").execute_test().command_is_successful()

# ************ Cleanup bucket + Object  ************
S3cmdTest('s3cmd can delete 3k file').delete_test("seagatebucket", "3kfile").execute_test().command_is_successful()
S3cmdTest('s3cmd can delete 18MB file').delete_test("seagatebucket", "18MBfile").execute_test().command_is_successful()
S3cmdTest('s3cmd can delete bucket').delete_bucket("seagatebucket").execute_test().command_is_successful()

# ******************* multipart and partial parts *********************
S3cmdTest('s3cmd can create bucket').create_bucket("seagatebucket").execute_test().command_is_successful()
JClientTest('Jclient can upload partial parts to test abort and list multipart.').partial_multipart_upload("seagatebucket", "18MBfile", 18000000, 1, 2).execute_test().command_is_successful()

S3fiTest('s3cmd enable FI GET KV').enable_fi("enable", "always", "clovis_kv_get_fail").execute_test().command_is_successful()
S3cmdTest('s3cmd can not list multipart uploads in progress').list_multipart_uploads("seagatebucket").execute_test(True).command_should_fail()
S3fiTest('s3cmd disable Fault injection').disable_fi("clovis_kv_get_fail").execute_test().command_is_successful()
result = S3cmdTest('s3cmd can list multipart uploads in progress').list_multipart_uploads("seagatebucket").execute_test()
result.command_response_should_have('18MBfile')
upload_id = result.status.stdout.split('\n')[2].split('\t')[2]

S3fiTest('s3cmd enable FI GET KV').enable_fi("enable", "always", "clovis_kv_get_fail").execute_test().command_is_successful()
result = S3cmdTest('S3cmd can not list parts of multipart upload.').list_parts("seagatebucket", "18MBfile", upload_id).execute_test(True).command_should_fail()
S3fiTest('s3cmd disable Fault injection').disable_fi("clovis_kv_get_fail").execute_test().command_is_successful()

S3fiTest('s3cmd enable FI GET KV').enable_fi("enable", "always", "clovis_kv_get_fail").execute_test().command_is_successful()
S3cmdTest('S3cmd can not abort multipart upload').abort_multipart("seagatebucket", "18MBfile", upload_id).execute_test(True).command_should_fail()
S3fiTest('s3cmd disable Fault injection').disable_fi("clovis_kv_get_fail").execute_test().command_is_successful()
S3cmdTest('S3cmd can abort multipart upload').abort_multipart("seagatebucket", "18MBfile", upload_id).execute_test().command_is_successful()

S3cmdTest('s3cmd can test the multipart was aborted.').list_multipart_uploads('seagatebucket').execute_test().command_is_successful().command_response_should_not_have('18MBfile')
S3cmdTest('s3cmd can delete bucket').delete_bucket("seagatebucket").execute_test().command_is_successful()
# ******************************************************************

# *************** Unused FI APIs above *************
# NOTE: Remove FI API if they are used in any test above in future
S3fiTest('s3cmd enable FI GET KV').enable_fi_random("enable", "unused_fail", "10").execute_test().command_is_successful()
S3fiTest('s3cmd enable FI GET KV').disable_fi("unused_fail").execute_test().command_is_successful()
S3fiTest('s3cmd enable FI GET KV').enable_fi_offnonm("enable", "unused_fail", "3", "5").execute_test().command_is_successful()
S3fiTest('s3cmd enable FI GET KV').disable_fi("unused_fail").execute_test().command_is_successful()
S3fiTest('s3cmd enable FI GET KV').enable_fi("enable", "once", "unused_fail").execute_test().command_is_successful()
S3fiTest('s3cmd enable FI GET KV').disable_fi("unused_fail").execute_test().command_is_successful()

# ************ Negative ACL/Policy TESTS ************
S3cmdTest('s3cmd can create bucket').create_bucket("seagatebucket").execute_test().command_is_successful()
S3cmdTest('s3cmd can upload 3k file with default acl').upload_test("seagatebucket", "3kfile", 3000).execute_test().command_is_successful()
S3fiTest('s3cmd enable FI PUT KV').enable_fi("enable", "always", "clovis_kv_put_fail").execute_test().command_is_successful()
S3cmdTest('s3cmd can not set acl on bucket').setacl_bucket("seagatebucket","read:123").execute_test(True).command_should_fail()
S3cmdTest('s3cmd can not set acl on object').setacl_object("seagatebucket","3kfile", "read:123").execute_test(True).command_should_fail()
S3cmdTest('s3cmd can not revoke acl on bucket').revoke_acl_bucket("seagatebucket","read:123").execute_test(True).command_should_fail()
S3cmdTest('s3cmd can not revoke acl on object').revoke_acl_object("seagatebucket","3kfile","read:123").execute_test(True).command_should_fail()
S3cmdTest('s3cmd can not set policy on bucket').setpolicy_bucket("seagatebucket","policy.txt").execute_test(True).command_should_fail()
S3fiTest('s3cmd disable Fault injection').disable_fi("clovis_kv_put_fail").execute_test().command_is_successful()
S3cmdTest('s3cmd can delete 3kfile after setting acl').delete_test("seagatebucket", "3kfile").execute_test().command_is_successful()
S3cmdTest('s3cmd can delete bucket after setting policy/acl').delete_bucket("seagatebucket").execute_test().command_is_successful()
# ************************************************

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
