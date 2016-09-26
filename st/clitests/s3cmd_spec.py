#!/usr/bin/python

from framework import Config
from framework import S3PyCliTest
from s3cmd import S3cmdTest
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

# ************ 3k FILE TEST ************
S3cmdTest('s3cmd can upload 3k file').upload_test("seagatebucket", "3kfile", 3000).execute_test().command_is_successful()

S3cmdTest('s3cmd can download 3k file').download_test("seagatebucket", "3kfile").execute_test().command_is_successful().command_created_file("3kfile")

S3cmdTest('s3cmd cannot copy object 3k file').upload_copy_test("seagatebucket", "3kfile", "3kfile.copy").execute_test(negative_case=True).command_should_fail()

S3cmdTest('s3cmd cannot move object 3k file').upload_move_test("seagatebucket", "3kfile", "seagatebucket", "3kfile.moved").execute_test(negative_case=True).command_should_fail()

# ************ 3k FILE TEST ************
S3cmdTest('s3cmd can upload 3k file with special chars in filename.').upload_test("seagatebucket/2016-04:32:21/3kfile", "3kfile", 3000).execute_test().command_is_successful()

S3cmdTest('s3cmd can download 3k file with special chars in filename.').download_test("seagatebucket/2016-04:32:21", "3kfile").execute_test().command_is_successful().command_created_file("3kfile")


# ************ 8k FILE TEST ************
S3cmdTest('s3cmd can upload 8k file').upload_test("seagatebucket", "8kfile", 8192).execute_test().command_is_successful()

S3cmdTest('s3cmd can download 8k file').download_test("seagatebucket", "8kfile").execute_test().command_is_successful().command_created_file("8kfile")

# ************ OBJECT LISTING TEST ************
S3cmdTest('s3cmd can list objects').list_objects('seagatebucket').execute_test().command_is_successful().command_response_should_have('s3://seagatebucket/3kfile').command_response_should_have('s3://seagatebucket/8kfile')

S3cmdTest('s3cmd can list specific objects').list_specific_objects('seagatebucket', '3k').execute_test().command_is_successful().command_response_should_have('s3://seagatebucket/3kfile').command_response_should_not_have('s3://seagatebucket/8kfile')

S3cmdTest('s3cmd can create bucket').create_bucket("seagatebucket2").execute_test().command_is_successful()

S3cmdTest('s3cmd can upload 8k file').upload_test("seagatebucket2", "8kfile", 8192).execute_test().command_is_successful()

S3cmdTest('s3cmd can list all objects').list_all_objects().execute_test().command_is_successful().command_response_should_have('s3://seagatebucket/3kfile').command_response_should_have('s3://seagatebucket2/8kfile')

# ************ Disk Usage TEST ************
S3cmdTest('s3cmd can show disk usage').disk_usage_bucket("seagatebucket").execute_test().command_is_successful()

# ************ DELETE OBJECT TEST ************
S3cmdTest('s3cmd can delete 3k file').delete_test("seagatebucket", "3kfile").execute_test().command_is_successful()

S3cmdTest('s3cmd can delete 3k file').delete_test("seagatebucket/2016-04:32:21", "3kfile").execute_test().command_is_successful()


S3cmdTest('s3cmd can delete 8k file').delete_test("seagatebucket", "8kfile").execute_test().command_is_successful()

S3cmdTest('s3cmd can delete 8k file').delete_test("seagatebucket2", "8kfile").execute_test().command_is_successful()

# ************ 700K FILE TEST ************
S3cmdTest('s3cmd can upload 700K file').upload_test("seagatebucket", "700Kfile", 716800).execute_test().command_is_successful()

S3cmdTest('s3cmd can download 700K file').download_test("seagatebucket", "700Kfile").execute_test().command_is_successful().command_created_file("700Kfile")

S3cmdTest('s3cmd can delete 700K file').delete_test("seagatebucket", "700Kfile").execute_test().command_is_successful()

# ************ 18MB FILE Multipart Upload TEST ***********
S3cmdTest('s3cmd can upload 18MB file').upload_test("seagatebucket", "18MBfile", 18000000).execute_test().command_is_successful()

S3cmdTest('s3cmd can download 18MB file').download_test("seagatebucket", "18MBfile").execute_test().command_is_successful().command_created_file("18MBfile")

S3cmdTest('s3cmd can delete 18MB file').delete_test("seagatebucket", "18MBfile").execute_test().command_is_successful()

#################################################
JClientTest('Jclient can upload partial parts to test abort and list multipart.').partial_multipart_upload("seagatebucket", "18MBfile", 18000000, 1, 2).execute_test().command_is_successful()

result = S3cmdTest('s3cmd can list multipart uploads in progress').list_multipart_uploads("seagatebucket").execute_test()
result.command_response_should_have('18MBfile')

upload_id = result.status.stdout.split('\n')[2].split('\t')[2]

result = S3cmdTest('S3cmd can list parts of multipart upload.').list_parts("seagatebucket", "18MBfile", upload_id).execute_test().command_is_successful()
assert len(result.status.stdout.split('\n')) == 4

S3cmdTest('S3cmd can abort multipart upload').abort_multipart("seagatebucket", "18MBfile", upload_id).execute_test().command_is_successful()

S3cmdTest('s3cmd can test the multipart was aborted.').list_multipart_uploads('seagatebucket').execute_test().command_is_successful().command_response_should_not_have('18MBfile')

S3cmdTest('s3cmd cannot list parts from a nonexistent bucket').list_parts("seagate-bucket", "18MBfile", upload_id).execute_test(negative_case=True).command_should_fail().command_error_should_have("NoSuchBucket")

S3cmdTest('s3cmd abort on nonexistent bucket should fail').abort_multipart("seagate-bucket", "18MBfile", upload_id).execute_test(negative_case=True).command_should_fail().command_error_should_have("NoSuchBucket")

#################################################

# ************ Multiple Delete bucket TEST ************
file_name = "3kfile"
for num in range(0, 4):
  new_file_name = '%s%d' % (file_name, num)
  S3cmdTest('s3cmd can upload 3k file').upload_test("seagatebucket", new_file_name, 3000).execute_test().command_is_successful()

S3cmdTest('s3cmd can delete multiple objects').multi_delete_test("seagatebucket").execute_test().command_is_successful().command_response_should_have('delete: \'s3://seagatebucket/3kfile0\'').command_response_should_have('delete: \'s3://seagatebucket/3kfile3\'')

S3cmdTest('s3cmd should not have objects after multiple delete').list_objects('seagatebucket').execute_test().command_is_successful().command_response_should_not_have('3kfile')

# ************ Delete bucket TEST ************
S3cmdTest('s3cmd can delete bucket').delete_bucket("seagatebucket").execute_test().command_is_successful()
S3cmdTest('s3cmd can delete bucket').delete_bucket("seagatebucket2").execute_test().command_is_successful()

# ************ Signing algorithm test ************
S3cmdTest('s3cmd can create bucket nondnsbucket').create_bucket("nondnsbucket").execute_test().command_is_successful()
S3cmdTest('s3cmd can create bucket seagate-bucket').create_bucket("seagate-bucket").execute_test().command_is_successful()
S3cmdTest('s3cmd can create bucket seagatebucket123').create_bucket("seagatebucket123").execute_test().command_is_successful()
S3cmdTest('s3cmd can create bucket seagate.bucket').create_bucket("seagate.bucket").execute_test().command_is_successful()
S3cmdTest('s3cmd can delete bucket nondnsbucket').delete_bucket("nondnsbucket").execute_test().command_is_successful()
S3cmdTest('s3cmd can delete bucket seagate-bucket').delete_bucket("seagate-bucket").execute_test().command_is_successful()
S3cmdTest('s3cmd can delete bucket seagatebucket123').delete_bucket("seagatebucket123").execute_test().command_is_successful()
S3cmdTest('s3cmd can delete bucket seagate.bucket').delete_bucket("seagate.bucket").execute_test().command_is_successful()

# ************ Create bucket in region ************
S3cmdTest('s3cmd can create bucket').create_bucket("seagatebucket", "eu-west-1").execute_test().command_is_successful()
S3cmdTest('s3cmd cannot fetch info for nonexistent bucket').info_bucket("seagate-bucket").execute_test(negative_case=True).command_should_fail().command_error_should_have("NoSuchBucket")
S3cmdTest('s3cmd created bucket in specific region').info_bucket("seagatebucket").execute_test().command_is_successful().command_response_should_have('eu-west-1')
S3cmdTest('s3cmd can upload 3k file').upload_test("seagatebucket", "3kfile", 3000).execute_test().command_is_successful()
S3cmdTest('s3cmd can retrieve obj info').info_object("seagatebucket", "3kfile").execute_test().command_is_successful().command_response_should_have('3kfile')
S3cmdTest('s3cmd can delete 3k file').delete_test("seagatebucket", "3kfile").execute_test().command_is_successful()
S3cmdTest('s3cmd can delete bucket').delete_bucket("seagatebucket").execute_test().command_is_successful()


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

# ************ 18MB FILE Multipart Upload TEST ***********
S3cmdTest('s3cmd can multipart upload 18MB file').upload_test("seagatebucket", "18MBfile", 18000000).execute_test().command_is_successful()

S3cmdTest('s3cmd can download 18MB file').download_test("seagatebucket", "18MBfile").execute_test().command_is_successful().command_created_file("18MBfile")

S3cmdTest('s3cmd can delete 18MB file').delete_test("seagatebucket", "18MBfile").execute_test().command_is_successful()

#################################################

JClientTest('Jclient can upload partial parts to test abort and list multipart.').partial_multipart_upload("seagatebucket", "18MBfile", 18000000, 1, 2).execute_test().command_is_successful()

result = S3cmdTest('s3cmd can list multipart uploads in progress').list_multipart_uploads("seagatebucket").execute_test()
result.command_response_should_have('18MBfile')

upload_id = result.status.stdout.split('\n')[2].split('\t')[2]

result = S3cmdTest('S3cmd can list parts of multipart upload.').list_parts("seagatebucket", "18MBfile", upload_id).execute_test().command_is_successful()
assert len(result.status.stdout.split('\n')) == 4

S3cmdTest('S3cmd can abort multipart upload').abort_multipart("seagatebucket", "18MBfile", upload_id).execute_test().command_is_successful()

S3cmdTest('s3cmd can test the multipart was aborted.').list_multipart_uploads('seagatebucket').execute_test().command_is_successful().command_response_should_not_have('18MBfile')

S3cmdTest('s3cmd cannot list parts from a nonexistent bucket').list_parts("seagate-bucket", "18MBfile", upload_id).execute_test(negative_case=True).command_should_fail().command_error_should_have("NoSuchBucket")

S3cmdTest('s3cmd abort on nonexistent bucket should fail').abort_multipart("seagate-bucket", "18MBfile", upload_id).execute_test(negative_case=True).command_should_fail().command_error_should_have("NoSuchBucket")


############################################

# S3cmdTest('s3cmd can delete 18MB file').delete_test("seagatebucket", "18MBfile").execute_test().command_is_successful()
#
# S3cmdTest('s3cmd can abort multipart upload of 18MB file').multipartupload_abort_test("seagatebucket", "18MBfile", 18000000).execute_test(negative_case=True).command_should_fail()
#
# S3cmdTest('s3cmd can list parts of multipart upload 18MB file').multipartupload_partlist_test("seagatebucket", "18MBfile", 18000000).execute_test().command_is_successful()
#
# S3cmdTest('s3cmd can delete 18MB file').delete_test("seagatebucket", "18MBfile").execute_test().command_is_successful()

# ************ Multiple Delete bucket TEST ************
file_name = "3kfile"
for num in range(0, 4):
  new_file_name = '%s%d' % (file_name, num)
  S3cmdTest('s3cmd can upload 3k file').upload_test("seagatebucket", new_file_name, 3000).execute_test().command_is_successful()

S3cmdTest('s3cmd can delete multiple objects').multi_delete_test("seagatebucket").execute_test().command_is_successful().command_response_should_have('delete: \'s3://seagatebucket/3kfile0\'').command_response_should_have('delete: \'s3://seagatebucket/3kfile3\'')

S3cmdTest('s3cmd should not have objects after multiple delete').list_objects('seagatebucket').execute_test().command_is_successful().command_response_should_not_have('3kfile')

# ************ Delete bucket TEST ************
S3cmdTest('s3cmd can delete bucket').delete_bucket("seagatebucket").execute_test().command_is_successful()

# ************ Signing algorithm test ************
S3cmdTest('s3cmd can create bucket seagate-bucket').create_bucket("seagate-bucket").execute_test().command_is_successful()
S3cmdTest('s3cmd can create bucket seagatebucket123').create_bucket("seagatebucket123").execute_test().command_is_successful()
S3cmdTest('s3cmd can create bucket seagate.bucket').create_bucket("seagate.bucket").execute_test().command_is_successful()
S3cmdTest('s3cmd can delete bucket seagate-bucket').delete_bucket("seagate-bucket").execute_test().command_is_successful()
S3cmdTest('s3cmd can delete bucket seagatebucket123').delete_bucket("seagatebucket123").execute_test().command_is_successful()
S3cmdTest('s3cmd can delete bucket seagate.bucket').delete_bucket("seagate.bucket").execute_test().command_is_successful()

# ************ Create bucket in region ************
S3cmdTest('s3cmd can create bucket').create_bucket("seagatebucket", "eu-west-1").execute_test().command_is_successful()

S3cmdTest('s3cmd created bucket in specific region').info_bucket("seagatebucket").execute_test().command_is_successful().command_response_should_have('eu-west-1')

S3cmdTest('s3cmd cannot fetch info for nonexistent bucket').info_bucket("seagate-bucket").execute_test(negative_case=True).command_should_fail().command_error_should_have("NoSuchBucket")

S3cmdTest('s3cmd can delete bucket').delete_bucket("seagatebucket").execute_test().command_is_successful()

# ************ Collision Resolution TEST ************
S3cmdTest('s3cmd can create bucket').create_bucket("seagatebucket").execute_test().command_is_successful()
S3cmdTest('s3cmd can upload 3k file for Collision resolution test').upload_test("seagatebucket", "3kfile", 3000).execute_test().command_is_successful()

S3cmdTest('Deleted metadata using cqlsh for Collision resolution test').delete_metadata_test().execute_test().command_is_successful().command_is_successful()

S3cmdTest('Create bucket for Collision resolution test').create_bucket("seagatebucket").execute_test().command_is_successful()

S3cmdTest('s3cmd can upload 3k file after Collision resolution').upload_test("seagatebucket", "3kfile", 3000).execute_test().command_is_successful()

S3cmdTest('Check metadata have key 3kfile after Collision resolution').get_keyval_test().execute_test().command_is_successful().command_response_should_have('3kfile')

S3cmdTest('s3cmd can download 3kfile after Collision resolution upload').download_test("seagatebucket", "3kfile").execute_test().command_is_successful().command_created_file("3kfile")

S3cmdTest('s3cmd can delete 3kfile after collision resolution').delete_test("seagatebucket", "3kfile").execute_test().command_is_successful()

S3cmdTest('s3cmd can delete bucket').delete_bucket("seagatebucket").execute_test().command_is_successful()

# ************ ACL/Policy TESTS ************
S3cmdTest('s3cmd can create bucket').create_bucket("seagatebucket").execute_test().command_is_successful()
S3cmdTest('Check for default bucket ACL').get_metadata().execute_test().command_is_successful().command_response_should_have('seagatebucket | {\"ACL\":\"PD94bWwgdmVyc2lvbj0iMS4wIiBlbmNvZGluZz0iVVRGLTgiPz4KPEFjY2Vzc0NvbnRyb2xQb2xpY3kgeG1sbnM9Imh0dHA6Ly9zMy5hbWF6b25hd3MuY29tL2RvYy8yMDA2LTAzLTAxLyI+CiAgPE93bmVyPgogICAgPElEPjEyMzQ1PC9JRD4KICA8L093bmVyPgogIDxBY2Nlc3NDb250cm9sTGlzdD4KICAgIDxHcmFudD4KICAgICAgPEdyYW50ZWUgeG1sbnM6eHNpPSJodHRwOi8vd3d3LnczLm9yZy8yMDAxL1hNTFNjaGVtYS1pbnN0YW5jZSIgeHNpOnR5cGU9IkNhbm9uaWNhbFVzZXIiPgogICAgICAgIDxJRD4xMjM0NTwvSUQ+CiAgICAgICAgPERpc3BsYXlOYW1lPnMzX3Rlc3Q8L0Rpc3BsYXlOYW1lPgogICAgICA8L0dyYW50ZWU+CiAgICAgIDxQZXJtaXNzaW9uPkZVTExfQ09OVFJPTDwvUGVybWlzc2lvbj4KICAgIDwvR3JhbnQ+CiAgPC9BY2Nlc3NDb250cm9sTGlzdD4KPC9BY2Nlc3NDb250cm9sUG9saWN5Pgo=')
S3cmdTest('s3cmd can upload 3k file with default acl').upload_test("seagatebucket", "3kfile", 3000).execute_test().command_is_successful()
S3cmdTest('Check for default object ACL').get_metadata().execute_test().command_is_successful().command_response_should_have('3kfile | {\"ACL\":\"PD94bWwgdmVyc2lvbj0iMS4wIiBlbmNvZGluZz0iVVRGLTgiPz4KPEFjY2Vzc0NvbnRyb2xQb2xpY3kgeG1sbnM9Imh0dHA6Ly9zMy5hbWF6b25hd3MuY29tL2RvYy8yMDA2LTAzLTAxLyI+CiAgPE93bmVyPgogICAgPElEPjEyMzQ1PC9JRD4KICAgICAgPERpc3BsYXlOYW1lPnMzX3Rlc3Q8L0Rpc3BsYXlOYW1lPgogIDwvT3duZXI+CiAgPEFjY2Vzc0NvbnRyb2xMaXN0PgogICAgPEdyYW50PgogICAgICA8R3JhbnRlZSB4bWxuczp4c2k9Imh0dHA6Ly93d3cudzMub3JnLzIwMDEvWE1MU2NoZW1hLWluc3RhbmNlIiB4c2k6dHlwZT0iQ2Fub25pY2FsVXNlciI+CiAgICAgICAgPElEPjEyMzQ1PC9JRD4KICAgICAgICA8RGlzcGxheU5hbWU+czNfdGVzdDwvRGlzcGxheU5hbWU+CiAgICAgIDwvR3JhbnRlZT4KICAgICAgPFBlcm1pc3Npb24+RlVMTF9DT05UUk9MPC9QZXJtaXNzaW9uPgogICAgPC9HcmFudD4KICA8L0FjY2Vzc0NvbnRyb2xMaXN0Pgo8L0FjY2Vzc0NvbnRyb2xQb2xpY3k+Cg==')
S3cmdTest('s3cmd can set acl on bucket').setacl_bucket("seagatebucket","read:123").execute_test().command_is_successful()
S3cmdTest('Check for default bucket ACL').get_metadata().execute_test().command_is_successful().command_response_should_have('{\"ACL\":\"PD94bWwgdmVyc2lvbj0iMS4wIiBlbmNvZGluZz0iVVRGLTgiPz4KPEFjY2Vzc0NvbnRyb2xQb2xpY3kgeG1sbnM9Imh0dHA6Ly9zMy5hbWF6b25hd3MuY29tL2RvYy8yMDA2LTAzLTAxLyI+PE93bmVyPjxJRD4xMjM0NTwvSUQ+PERpc3BsYXlOYW1lPnMzX3Rlc3Q8L0Rpc3BsYXlOYW1lPj48L093bmVyPjxBY2Nlc3NDb250cm9sTGlzdD48R3JhbnQ+PEdyYW50ZWUgeG1sbnM6eHNpPSJodHRwOi8vd3d3LnczLm9yZy8yMDAxL1hNTFNjaGVtYS1pbnN0YW5jZSIgeHNpOnR5cGU9IkNhbm9uaWNhbFVzZXIiPjxJRD4xMjM0NTwvSUQ+PERpc3BsYXlOYW1lPnMzX3Rlc3Q8L0Rpc3BsYXlOYW1lPj48L0dyYW50ZWU+PFBlcm1pc3Npb24+RlVMTF9DT05UUk9MPC9QZXJtaXNzaW9uPjwvR3JhbnQ+PEdyYW50PjxHcmFudGVlIHhtbG5zOnhzaT0iaHR0cDovL3d3dy53My5vcmcvMjAwMS9YTUxTY2hlbWEtaW5zdGFuY2UiIHhzaTp0eXBlPSJDYW5vbmljYWxVc2VyIj48SUQ+MTIzPC9JRD48RGlzcGxheU5hbWU+czNfdGVzdDwvRGlzcGxheU5hbWU+PjwvR3JhbnRlZT48UGVybWlzc2lvbj5SRUFEPC9QZXJtaXNzaW9uPjwvR3JhbnQ+PC9BY2Nlc3NDb250cm9sTGlzdD48L0FjY2Vzc0NvbnRyb2xQb2xpY3k+\",\"Bucket-Name\":\"seagatebucket\",\"Policy\":\"\"')
S3cmdTest('s3cmd can set acl on object').setacl_object("seagatebucket","3kfile", "read:123").execute_test().command_is_successful()
S3cmdTest('Check for object ACL').get_metadata().execute_test().command_is_successful().command_response_should_have('{\"ACL\":\"PEFjY2Vzc0NvbnRyb2xQb2xpY3kgeG1sbnM9Imh0dHA6Ly9zMy5hbWF6b25hd3MuY29tL2RvYy8yMDA2LTAzLTAxLyI+PE93bmVyPjxJRD4xMjM0NTwvSUQ+PERpc3BsYXlOYW1lPnMzX3Rlc3Q8L0Rpc3BsYXlOYW1lPj48L093bmVyPjxBY2Nlc3NDb250cm9sTGlzdD48R3JhbnQ+PEdyYW50ZWUgeG1sbnM6eHNpPSJodHRwOi8vd3d3LnczLm9yZy8yMDAxL1hNTFNjaGVtYS1pbnN0YW5jZSIgeHNpOnR5cGU9IkNhbm9uaWNhbFVzZXIiPjxJRD4xMjM0NTwvSUQ+PERpc3BsYXlOYW1lPnMzX3Rlc3Q8L0Rpc3BsYXlOYW1lPj48L0dyYW50ZWU+PFBlcm1pc3Npb24+RlVMTF9DT05UUk9MPC9QZXJtaXNzaW9uPjwvR3JhbnQ+PEdyYW50PjxHcmFudGVlIHhtbG5zOnhzaT0iaHR0cDovL3d3dy53My5vcmcvMjAwMS9YTUxTY2hlbWEtaW5zdGFuY2UiIHhzaTp0eXBlPSJDYW5vbmljYWxVc2VyIj48SUQ+MTIzPC9JRD48RGlzcGxheU5hbWU+czNfdGVzdDwvRGlzcGxheU5hbWU+PjwvR3JhbnRlZT48UGVybWlzc2lvbj5SRUFEPC9QZXJtaXNzaW9uPjwvR3JhbnQ+PC9BY2Nlc3NDb250cm9sTGlzdD48L0FjY2Vzc0NvbnRyb2xQb2xpY3k+\",\"Bucket-Name\":\"seagatebucket\",\"Object-Name\":\"3kfile\"')
S3cmdTest('s3cmd can revoke acl on bucket').revoke_acl_bucket("seagatebucket","read:123").execute_test().command_is_successful()
S3cmdTest('s3cmd can revoke acl on object').revoke_acl_object("seagatebucket","3kfile","read:123").execute_test().command_is_successful()
S3cmdTest('s3cmd can set policy on bucket').setpolicy_bucket("seagatebucket","policy.txt").execute_test().command_is_successful()
S3cmdTest('s3cmd can delete 3kfile after setting acl').delete_test("seagatebucket", "3kfile").execute_test().command_is_successful()
S3cmdTest('s3cmd can set policy on bucket').delpolicy_bucket("seagatebucket").execute_test().command_is_successful()
S3cmdTest('s3cmd can delete bucket after setting policy/acl').delete_bucket("seagatebucket").execute_test().command_is_successful()
