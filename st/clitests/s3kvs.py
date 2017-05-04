# Helper defines for metadata operations
import sys
import json
import base64
from s3kvstool import S3kvTest, S3OID

# Find record in record list, return empty string otherwise
def _find_record(fkey, record_list):
    for entry in record_list:
        if fkey in entry:
            return entry
    return ""

# Extract json formatted value from given record
def _find_keyval_json(record):
    record_split = record.split('\n')
    for entry in record_split:
        if entry.startswith("Val:"):
            json_keyval_string = entry[5:]
            return json_keyval_string
    return ""

# Extract object id for bucket/object
def _extract_oid(json_keyval, bucket=True):
    keyval = json.loads(json_keyval)
    sbyteorder = sys.byteorder
    oid_val = S3OID()
    if bucket:
        dec_string_oid_hi = base64.b64decode(keyval['bucket_list_index_oid_u_hi'])
        dec_string_oid_lo = base64.b64decode(keyval['bucket_list_index_oid_u_lo'])
    else:
        dec_string_oid_hi = base64.b64decode(keyval['mero_object_list_index_oid_u_hi'])
        dec_string_oid_lo = base64.b64decode(keyval['mero_object_list_index_oid_u_lo'])
    int_oid_hi = int.from_bytes(dec_string_oid_hi,byteorder=sbyteorder)
    int_oid_lo = int.from_bytes(dec_string_oid_lo,byteorder=sbyteorder)
    oid_val.set_oid(hex(int_oid_hi), hex(int_oid_lo))
    return oid_val

# Helper to fetch System Test user record from kvs
def _fetch_test_user_info():
    result = S3kvTest('kvtest can list user records').root_index_records().execute_test(ignore_err=True)
    root_index_records = result.status.stdout.split('----------------------------------------------')
    test_record_key =  "ACCOUNTUSER/s3_test/tester"
    test_user_record = _find_record(test_record_key, root_index_records)
    return test_user_record

# Fetch given bucket record for System Test user account
def _fetch_bucket_info(bucket_name):
    test_user_record = _fetch_test_user_info()
    test_user_json_keyval = _find_keyval_json(test_user_record)
    oid_decoded = _extract_oid(test_user_json_keyval, bucket=True)
    result = S3kvTest('Kvtest list user bucket(s)').next_keyval(oid_decoded, "", 5).execute_test(ignore_err=True)
    test_user_bucket_list = result.status.stdout.split('----------------------------------------------')
    bucket_record = _find_record(bucket_name, test_user_bucket_list)
    return bucket_record

# Fetch bucket record, assert on failure
def _fail_fetch_bucket_info(bucket_name):
    bucket_record = _fetch_bucket_info(bucket_name)
    assert bucket_record,"bucket:%s not found!" % bucket_name
    return bucket_record

# Given bucket record, fetch key value pair, if exist
def _fetch_object_info(key_name, bucket_record):
    bucket_json_keyval = _find_keyval_json(bucket_record)
    oid_decoded = _extract_oid(bucket_json_keyval, bucket=False)
    result = S3kvTest('Kvtest list user bucket(s)').next_keyval(oid_decoded, "", 10).execute_test(ignore_err=True)
    bucket_entries = result.status.stdout.split('----------------------------------------------')
    file_record = _find_record(key_name, bucket_entries)
    return file_record

# Test for record in bucket
def expect_object_in_bucket(bucket_name, key):
    bucket_record = _fail_fetch_bucket_info(bucket_name)
    file_record = _fetch_object_info(key, bucket_record)
    assert file_record,"key:%s not found in bucket %s!" % key % bucket_name
    return file_record

# Fetch acl from metadata for given bucket
def _fetch_bucket_acl(bucket_name):
    bucket_record = _fail_fetch_bucket_info(bucket_name)
    bucket_json_keyval = _find_keyval_json(bucket_record)
    bucket_keyval = json.loads(bucket_json_keyval)
    return bucket_keyval['ACL']


# Check if bucket is empty by checking against mero_object_list_index_oid_u_*
def _check_bucket_not_empty(bucket_record):
    default_empty_object_list_index_oid = "AAAAAAAAAAA="
    bucket_json_keyval = _find_keyval_json(bucket_record)
    bucket_keyval = json.loads(bucket_json_keyval)
    mero_object_list_index_oid_u_hi = bucket_keyval['mero_object_list_index_oid_u_hi']
    mero_object_list_index_oid_u_lo = bucket_keyval['mero_object_list_index_oid_u_lo']

    if (mero_object_list_index_oid_u_hi == default_empty_object_list_index_oid and
        mero_object_list_index_oid_u_lo == default_empty_object_list_index_oid):
        return False
    return True

# Fetch object acl from metadata
def _fetch_object_acl(bucket_name, key):
    file_record = expect_object_in_bucket(bucket_name, key)
    file_json_keyval = _find_keyval_json(file_record)
    file_keyval = json.loads(file_json_keyval)
    return file_keyval['ACL']

# Check against (default) acl for given bucket/object
def check_object_acl(bucket_name, key="", acl="", default_acl_test=False):
    default_acl = "PD94bWwgdmVyc2lvbj0iMS4wIiBlbmNvZGluZz0iVVRGLTgiPz4KPEFjY2Vzc0NvbnRyb2xQb2xpY3kgeG1sbnM9Imh0dHA6Ly9zMy5hbWF6b25hd3MuY29tL2RvYy8yMDA2LTAzLTAxLyI+CiAgPE93bmVyPgogICAgPElEPjEyMzwvSUQ+CiAgICAgIDxEaXNwbGF5TmFtZT50ZXN0ZXI8L0Rpc3BsYXlOYW1lPgogIDwvT3duZXI+CiAgPEFjY2Vzc0NvbnRyb2xMaXN0PgogICAgPEdyYW50PgogICAgICA8R3JhbnRlZSB4bWxuczp4c2k9Imh0dHA6Ly93d3cudzMub3JnLzIwMDEvWE1MU2NoZW1hLWluc3RhbmNlIiB4c2k6dHlwZT0iQ2Fub25pY2FsVXNlciI+CiAgICAgICAgPElEPjEyMzwvSUQ+CiAgICAgICAgPERpc3BsYXlOYW1lPnRlc3RlcjwvRGlzcGxheU5hbWU+CiAgICAgIDwvR3JhbnRlZT4KICAgICAgPFBlcm1pc3Npb24+RlVMTF9DT05UUk9MPC9QZXJtaXNzaW9uPgogICAgPC9HcmFudD4KICA8L0FjY2Vzc0NvbnRyb2xMaXN0Pgo8L0FjY2Vzc0NvbnRyb2xQb2xpY3k+Cg=="
    if default_acl_test:
        test_against_acl = default_acl
    else:
        test_against_acl = acl

    object_acl = _fetch_object_acl(bucket_name, key)

    if (object_acl == test_against_acl):
        print("Success")
        return
    else:
        raise AssertionError("Default ACL not matched for " + key + " in bucket " + bucket_name )
        return

# Check against (default) acl for given bucket/object
def check_bucket_acl(bucket_name, acl="", default_acl_test=False):
    default_acl = "PD94bWwgdmVyc2lvbj0iMS4wIiBlbmNvZGluZz0iVVRGLTgiPz4KPEFjY2Vzc0NvbnRyb2xQb2xpY3kgeG1sbnM9Imh0dHA6Ly9zMy5hbWF6b25hd3MuY29tL2RvYy8yMDA2LTAzLTAxLyI+CiAgPE93bmVyPgogICAgPElEPjEyMzwvSUQ+CiAgICAgIDxEaXNwbGF5TmFtZT50ZXN0ZXI8L0Rpc3BsYXlOYW1lPgogIDwvT3duZXI+CiAgPEFjY2Vzc0NvbnRyb2xMaXN0PgogICAgPEdyYW50PgogICAgICA8R3JhbnRlZSB4bWxuczp4c2k9Imh0dHA6Ly93d3cudzMub3JnLzIwMDEvWE1MU2NoZW1hLWluc3RhbmNlIiB4c2k6dHlwZT0iQ2Fub25pY2FsVXNlciI+CiAgICAgICAgPElEPjEyMzwvSUQ+CiAgICAgICAgPERpc3BsYXlOYW1lPnRlc3RlcjwvRGlzcGxheU5hbWU+CiAgICAgIDwvR3JhbnRlZT4KICAgICAgPFBlcm1pc3Npb24+RlVMTF9DT05UUk9MPC9QZXJtaXNzaW9uPgogICAgPC9HcmFudD4KICA8L0FjY2Vzc0NvbnRyb2xMaXN0Pgo8L0FjY2Vzc0NvbnRyb2xQb2xpY3k+Cg=="

    if default_acl_test:
        test_against_acl = default_acl
    else:
        test_against_acl = acl

    bucket_acl = _fetch_bucket_acl(bucket_name)

    if (bucket_acl == test_against_acl):
        print("Success")
        return
    else:
        raise AssertionError("Default ACL not matched for " + bucket_name)
        return

# Delete key from given index table
def delete_file_info(bucket_name,key):
    bucket_record = _fail_fetch_bucket_info(bucket_name)
    bucket_json_keyval = _find_keyval_json(bucket_record)
    oid_decoded = _extract_oid(bucket_json_keyval, bucket=False)
    result = S3kvTest('Kvtest delete key from bucket').delete_keyval(oid_decoded,key).execute_test(ignore_err=True)
    file_record = _fetch_object_info(key,bucket_record)
    assert not file_record,"key:%s not deleted from bucket %s!" % key % bucket_name
    return file_record

# Delete bucket record
def delete_bucket_info(bucket_name):
    bucket_record = _fail_fetch_bucket_info(bucket_name)
    bucket_json_keyval = _find_keyval_json(bucket_record)
    oid_decoded = _extract_oid(bucket_json_keyval, bucket=False)
    # Check if bucket is empty
    if _check_bucket_not_empty(bucket_record):
        result = S3kvTest('Kvtest delete bucket record').delete_index(oid_decoded).execute_test(ignore_err=True)

    # delete bucket record from test user bucket list
    test_user_record = _fetch_test_user_info()
    test_user_json_keyval = _find_keyval_json(test_user_record)
    oid_decoded = _extract_oid(test_user_json_keyval,bucket=True)
    result = S3kvTest('Kvtest delete bucket').delete_keyval(oid_decoded,bucket_name).execute_test(ignore_err=True)
    bucket_record = _fetch_bucket_info(bucket_name)
    assert not bucket_record,"bucket:%s entry not deleted!" % bucket_name
    return

# Delete User record
def delete_user_info(user_record="ACCOUNTUSER/s3_test/tester"):
    root_oid = S3kvTest('Kvtest fetch root index').root_index()
    result = S3kvTest('Kvtest delete user record').delete_keyval(root_oid,user_record).execute_test(ignore_err=True)
    return
