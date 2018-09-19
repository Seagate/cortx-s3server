#!/usr/bin/python3.4

from framework import Config
from framework import S3PyCliTest
from s3cmd import S3cmdTest
from s3fi import S3fiTest
from jclient import JClientTest
from s3client_config import S3ClientConfig
from s3kvstool import S3kvTest
import s3kvs

# Helps debugging
# Config.log_enabled = True
# Config.dummy_run = True
# Config.client_execution_timeout = 300 * 1000
# Config.request_timeout = 300 * 1000
# Config.socket_timeout = 300 * 1000
# Enable retry flag to limit retries on failure
Config.s3cmd_max_retries = 2

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

config_types = ["pathstyle.s3cfg", "virtualhoststyle.s3cfg"]
for i, type in enumerate(config_types):
    Config.config_file = type

    # Create bucket list index failure when creating bucket
    S3fiTest('s3cmd enable FI create index fail').enable_fi("enable", "always", "clovis_idx_create_fail").execute_test().command_is_successful()
    S3cmdTest('s3cmd cannot create bucket').create_bucket("seagatebucket").execute_test(negative_case=True).command_should_fail().command_error_should_have("InternalError")
    S3fiTest('s3cmd disable Fault injection').disable_fi("clovis_idx_create_fail").execute_test().command_is_successful()

    # Create object list index failure when creating bucket
    S3fiTest('s3cmd enable FI create index fail').enable_fi_offnonm("enable", "clovis_idx_create_fail", "1", "1").execute_test().command_is_successful()
    S3cmdTest('s3cmd cannot create bucket').create_bucket("seagatebucket").execute_test(negative_case=True).command_should_fail().command_error_should_have("InternalError")
    S3fiTest('s3cmd disable Fault injection').disable_fi("clovis_idx_create_fail").execute_test().command_is_successful()

    # Create multipart list index failure when creating bucket
    S3fiTest('s3cmd enable FI create index fail').enable_fi_offnonm("enable", "clovis_idx_create_fail", "2", "1").execute_test().command_is_successful()
    S3cmdTest('s3cmd cannot create bucket').create_bucket("seagatebucket").execute_test(negative_case=True).command_should_fail().command_error_should_have("InternalError")
    S3fiTest('s3cmd disable Fault injection').disable_fi("clovis_idx_create_fail").execute_test().command_is_successful()


    # ************ Create bucket ************
    S3cmdTest('s3cmd can create bucket').create_bucket("seagatebucket").\
        execute_test().command_is_successful()

    # ************ List buckets ************
    S3cmdTest('s3cmd can list buckets').list_buckets().execute_test().\
        command_is_successful().command_response_should_have('s3://seagatebucket')

    # ************ ACCOUNT USER IDX METADATA CORRUPTION TEST ***********
    # If Account user idx metadata is corrupted then bucket listing shall
    # return an error
    S3fiTest('s3cmd enable FI account_user_idx_metadata_corrupted').\
        enable_fi("enable", "always", "account_user_idx_metadata_corrupted").\
        execute_test().command_is_successful()
    S3cmdTest('s3cmd can not list bucket').list_buckets().\
        execute_test(negative_case=True).command_should_fail().\
        command_error_should_have("InternalError")
    S3fiTest('s3cmd can disable FI account_user_idx_metadata_corrupted').\
        disable_fi("account_user_idx_metadata_corrupted").\
        execute_test().command_is_successful()

    # ************ BUCKET METADATA CORRUPTION TEST ***********
    # Bucket listing shouldn't list corrupted bucket
    S3cmdTest('s3cmd can create bucket').create_bucket("seagatebucket_1").\
        execute_test().command_is_successful()
    S3fiTest('s3cmd enable FI bucket_metadata_corrupted').\
        enable_fi_enablen("enable", "bucket_metadata_corrupted", "2").\
        execute_test().command_is_successful()
    S3cmdTest('s3cmd does not list corrupted bucket').list_buckets().\
        execute_test().command_is_successful().\
        command_response_should_not_have('s3://seagatebucket_1').\
        command_response_should_have('s3://seagatebucket')
    S3fiTest('s3cmd can disable FI bucket_metadata_corrupted').\
        disable_fi("bucket_metadata_corrupted").\
        execute_test().command_is_successful()
    S3cmdTest('s3cmd can delete bucket').delete_bucket("seagatebucket_1").\
        execute_test().command_is_successful()

    # If bucket metadata is corrupted then object listing within bucket shall
    # return an error
    S3fiTest('s3cmd enable FI bucket_metadata_corrupted').\
        enable_fi("enable", "always", "bucket_metadata_corrupted").\
        execute_test().command_is_successful()
    S3cmdTest('s3cmd can not list objects within bucket').list_objects('seagatebucket').\
        execute_test(negative_case=True).command_should_fail().\
        command_error_should_have("InternalError")
    S3fiTest('s3cmd can disable FI bucket_metadata_corrupted').\
        disable_fi("bucket_metadata_corrupted").\
        execute_test().command_is_successful()

    # ************ OBJECT METADATA CORRUPTION TEST ***********
    # Object listing shouldn't list corrupted objects
    S3cmdTest('s3cmd can upload 3K file').\
        upload_test("seagatebucket", "3Kfile", 3000).\
        execute_test().command_is_successful()
    S3cmdTest('s3cmd can upload 9K file').\
        upload_test("seagatebucket", "9Kfile", 9000).\
        execute_test().command_is_successful()
    S3fiTest('s3cmd can enable FI object_metadata_corrupted').\
        enable_fi_enablen("enable", "object_metadata_corrupted", "2").\
        execute_test().command_is_successful()
    S3cmdTest('s3cmd does not list corrupted objects').list_objects('seagatebucket').\
        execute_test().command_is_successful().\
        command_response_should_not_have('9Kfile').\
        command_response_should_have('3Kfile')
    S3fiTest('s3cmd can disable FI object_metadata_corrupted').\
        disable_fi("object_metadata_corrupted").\
        execute_test().command_is_successful()
    S3cmdTest('s3cmd can delete 3K file').\
        delete_test("seagatebucket", "3Kfile").\
        execute_test().command_is_successful()
    S3cmdTest('s3cmd can delete 9K file').\
        delete_test("seagatebucket", "9Kfile").\
        execute_test().command_is_successful()

    # `Get Object` for corrupted object shall return an error
    S3cmdTest('s3cmd can upload 3K file').\
        upload_test("seagatebucket", "3Kfile", 3000).\
        execute_test().command_is_successful()
    S3fiTest('s3cmd can enable FI object_metadata_corrupted').\
        enable_fi("enable", "always", "object_metadata_corrupted").\
        execute_test().command_is_successful()
    S3cmdTest('s3cmd can not download corrupted object').\
        download_test("seagatebucket", "3Kfile").\
        execute_test(negative_case=True).command_should_fail()
    S3fiTest('s3cmd can disable FI object_metadata_corrupted').\
        disable_fi("object_metadata_corrupted").\
        execute_test().command_is_successful()
    S3cmdTest('s3cmd can delete 3K file').\
        delete_test("seagatebucket", "3Kfile").\
        execute_test().command_is_successful()

    # clovis_open_entity fails
    S3fiTest('s3cmd can enable FI clovis_enity_open').\
        enable_fi("enable", "always", "clovis_entity_open_fail").\
        execute_test().command_is_successful()
    S3cmdTest('s3cmd can not upload 18MBfile object').\
        upload_test("seagatebucket", "18MBfile", 18000000).\
        execute_test(negative_case=True).command_should_fail()

    # test for delete bucket with multipart object
    S3cmdTest('s3cmd cannot delete bucket').delete_bucket("seagatebucket").\
        execute_test(negative_case=True).command_should_fail().\
        command_error_should_have("ServiceUnavailable")

    S3fiTest('s3cmd can disable FI clovis_enity_open_fail').\
        disable_fi("clovis_entity_open_fail").\
        execute_test().command_is_successful()
    result = S3cmdTest('s3cmd can list multipart uploads in progress').\
             list_multipart_uploads("seagatebucket").execute_test()
    result.command_response_should_have('18MBfile')
    upload_id = result.status.stdout.split('\n')[2].split('\t')[2]

    result = S3cmdTest('S3cmd can list parts of multipart upload.').\
             list_parts("seagatebucket", "18MBfile", upload_id).\
             execute_test().command_is_successful()

    S3cmdTest('S3cmd can abort multipart upload').\
    abort_multipart("seagatebucket", "18MBfile", upload_id).\
    execute_test().command_is_successful()

    #clovis_enity_open failure and chunk upload
    S3fiTest('s3cmd can enable FI clovis_enity_open').\
        enable_fi("enable", "always", "clovis_entity_open_fail").\
        execute_test().command_is_successful()
    JClientTest('Jclient can upload 3k file in chunked mode').\
        put_object("seagatebucket", "3Kfile", 3000, chunked=True).\
        execute_test().command_is_successful()
    S3fiTest('s3cmd can disable FI clovis_enity_open_fail').\
        disable_fi("clovis_entity_open_fail").\
        execute_test().command_is_successful()
    S3cmdTest('s3cmd can delete 3k file').\
        delete_test("seagatebucket", "3Kfile").\
        execute_test().command_is_successful()



    # clovis_open_entity fails read failure
    S3cmdTest('s3cmd can upload 3K file').\
        upload_test("seagatebucket", "3Kfile", 3000).\
        execute_test().command_is_successful()
    S3fiTest('s3cmd can enable FI clovis_enity_open').\
        enable_fi("enable", "always", "clovis_entity_open_fail").\
        execute_test().command_is_successful()
    S3cmdTest('s3cmd cannot download 3k file').\
        download_test("seagatebucket", "3kfile").\
        execute_test(negative_case=True).command_should_fail()
    S3fiTest('s3cmd can disable FI clovis_enity_open_fail').\
        disable_fi("clovis_entity_open_fail").\
        execute_test().command_is_successful()
    S3cmdTest('s3cmd can delete 3k file').\
        delete_test("seagatebucket", "3Kfile").\
        execute_test().command_is_successful()


    # Multipart listing shall return an error for corrupted object
    JClientTest('Jclient can upload partial parts').\
        partial_multipart_upload("seagatebucket", "18MBfile", 18000000, 1, 2).\
        execute_test().command_is_successful()

    result = JClientTest('Jclient can list all multipart uploads.').\
                list_multipart("seagatebucket").execute_test()
    result.command_response_should_have('18MBfile')
    upload_id = result.status.stdout.split("id - ")[1]
    print(upload_id)

    S3fiTest('s3cmd can enable FI object_metadata_corrupted').\
        enable_fi("enable", "always", "object_metadata_corrupted").\
        execute_test().command_is_successful()
    JClientTest('Jclient can not list multipart uploads of corrupted object').\
        list_parts("seagatebucket", "18MBfile", upload_id).\
        execute_test(negative_case=True).command_should_fail().\
        command_error_should_have("InternalError")
    S3fiTest('s3cmd can disable FI object_metadata_corrupted').\
        disable_fi("object_metadata_corrupted").\
        execute_test().command_is_successful()
    JClientTest('Jclient can abort multipart upload').\
        abort_multipart("seagatebucket", "18MBfile", upload_id).\
        execute_test().command_is_successful()

    # ************ PART METADATA CORRUPTION TEST ***********
    # Multipart listing shouldn't list corrupted parts
    JClientTest('Jclient can upload partial parts').\
        partial_multipart_upload("seagatebucket", "18MBfile", 18000000, 1, 2).\
        execute_test().command_is_successful()

    result = JClientTest('Jclient can list all multipart uploads.').\
                list_multipart("seagatebucket").execute_test()
    result.command_response_should_have('18MBfile')
    upload_id = result.status.stdout.split("id - ")[1]
    print(upload_id)

    S3fiTest('s3cmd can enable FI part_metadata_corrupted').\
        enable_fi_enablen("enable", "part_metadata_corrupted", "2").\
        execute_test().command_is_successful()
    result = JClientTest('Jclient does not list corrupted part').\
        list_parts("seagatebucket", "18MBfile", upload_id).\
        execute_test()
    result.command_response_should_have("part number - 1").\
           command_response_should_not_have("part number - 2")
    S3fiTest('s3cmd can disable FI part_metadata_corrupted').\
        disable_fi("part_metadata_corrupted").\
        execute_test().command_is_successful()
    JClientTest('Jclient can abort multipart upload').\
        abort_multipart("seagatebucket", "18MBfile", upload_id).\
        execute_test().command_is_successful()

    # ************ Delete bucket ************
    S3cmdTest('s3cmd can delete bucket').delete_bucket("seagatebucket").\
        execute_test().command_is_successful()
