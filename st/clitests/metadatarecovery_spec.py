#!/usr/bin/python3.6
'''
 COPYRIGHT 2020 SEAGATE LLC

 THIS DRAWING/DOCUMENT, ITS SPECIFICATIONS, AND THE DATA CONTAINED
 HEREIN, ARE THE EXCLUSIVE PROPERTY OF SEAGATE TECHNOLOGY
 LIMITED, ISSUED IN STRICT CONFIDENCE AND SHALL NOT, WITHOUT
 THE PRIOR WRITTEN PERMISSION OF SEAGATE TECHNOLOGY LIMITED,
 BE REPRODUCED, COPIED, OR DISCLOSED TO A THIRD PARTY, OR
 USED FOR ANY PURPOSE WHATSOEVER, OR STORED IN A RETRIEVAL SYSTEM
 EXCEPT AS ALLOWED BY THE TERMS OF SEAGATE LICENSES AND AGREEMENTS.

 YOU SHOULD HAVE RECEIVED A COPY OF SEAGATE'S LICENSE ALONG WITH
 THIS RELEASE. IF NOT PLEASE CONTACT A SEAGATE REPRESENTATIVE
 http://www.seagate.com/contact

 Original author: Amit Kumar  <amit.kumar@seagate.com>
 Original creation date: 02-July-2020
'''

import sys
import os
import yaml
import shutil
import re
import json
from framework import Config
from framework import S3PyCliTest
from s3client_config import S3ClientConfig
from s3recoverytool import S3RecoveryTest
from auth import AuthTest
from awss3api import AwsTest
from s3kvstool import S3kvTest

from s3backgrounddelete.cortx_s3_config import CORTXS3Config
from s3backgrounddelete.cortx_s3_kv_api import CORTXS3KVApi
from s3backgrounddelete.cortx_s3_index_api import CORTXS3IndexApi
from s3backgrounddelete.cortx_list_index_response import CORTXS3ListIndexResponse
from s3backgrounddelete.cortx_s3_success_response import CORTXS3SuccessResponse

# Run before all to setup the test environment.
def before_all():
    print("Configuring LDAP")
    S3PyCliTest('Before_all').before_all()

# Run before all to setup the test environment.
before_all()

# Set basic properties
# Config.log_enabled = True
Config.config_file = "pathstyle.s3cfg"
S3ClientConfig.pathstyle = False
S3ClientConfig.ldapuser = 'sgiamadmin'
S3ClientConfig.ldappasswd = 'ldapadmin'

# Config files used by s3backgrounddelete
# We are using s3backgrounddelete config file as CORTXS3Config is tightly coupled with it.
origional_bgdelete_config_file = os.path.join(os.path.dirname(__file__), 's3_background_delete_config_test.yaml')
bgdelete_config_dir = os.path.join('/', 'opt', 'seagate', 'cortx', 's3', 's3backgrounddelete')
bgdelete_config_file = os.path.join(bgdelete_config_dir, 'config.yaml')
backup_bgdelete_config_file = os.path.join(bgdelete_config_dir, 'backup_config.yaml')


# Update S3 Background delete config file with account access key and secretKey.
def load_and_update_config(access_key_value, secret_key_value):
    # Update config file
    if os.path.isfile(bgdelete_config_file):
       shutil.copy2(bgdelete_config_file, backup_bgdelete_config_file)
    else:
       try:
           os.stat(bgdelete_config_dir)
       except:
           os.mkdir(bgdelete_config_dir)
       shutil.copy2(origional_bgdelete_config_file, bgdelete_config_file)

    with open(bgdelete_config_file, 'r') as f:
            config = yaml.safe_load(f)
            config['s3_recovery']['access_key'] = access_key_value
            config['s3_recovery']['secret_key'] = secret_key_value
            config['cortx_s3']['daemon_mode'] = "False"
            config['leakconfig']['leak_processing_delay_in_mins'] = 0
            config['leakconfig']['version_processing_delay_in_mins'] = 0

    with open(bgdelete_config_file, 'w') as f:
            yaml.dump(config, f)

    os.environ["AWS_ACCESS_KEY_ID"] = access_key_value
    os.environ["AWS_SECRET_ACCESS_KEY"] = secret_key_value

    # Update values of access key and secret key for s3iamcli commands
    S3ClientConfig.access_key_id = access_key_value
    S3ClientConfig.secret_key = secret_key_value


# Restore previous configurations
def restore_configuration():
    # Restore/Delete config file
    if os.path.isfile(backup_bgdelete_config_file):
       shutil.copy2(backup_bgdelete_config_file, bgdelete_config_file)
       os.remove(backup_bgdelete_config_file)
    else:
       os.remove(bgdelete_config_file)

    # Restore access key and secret key.
    del os.environ["AWS_ACCESS_KEY_ID"]
    del os.environ["AWS_SECRET_ACCESS_KEY"]

# *********************Create account s3-recovery-svc************************
test_msg = "Create account s3-recovery-svc"
account_args = {'AccountName': 's3-recovery-svc',\
                'Email': 's3-recovery-svc@seagate.com',\
                'ldapuser': S3ClientConfig.ldapuser,\
                'ldappasswd': S3ClientConfig.ldappasswd}
account_response_pattern = "AccountId = [\w-]*, CanonicalId = [\w-]*, RootUserName = [\w+=,.@-]*, AccessKeyId = [\w-]*, SecretKey = [\w/+]*$"
result = AuthTest(test_msg).create_account(**account_args).execute_test()
result.command_should_match_pattern(account_response_pattern)
account_response_elements = AuthTest.get_response_elements(result.status.stdout)
print(account_response_elements)


# ********** Update s3background delete config file with AccesskeyId and SecretKey**********************
load_and_update_config(account_response_elements['AccessKeyId'], account_response_elements['SecretKey'])

replica_bucket_list_index_oid = 'AAAAAAAAAHg=-BQAQAAAAAAA=' # base64 conversion of "0x7800000000000000" and "0x100005"
primary_bucket_list_index_oid = 'AAAAAAAAAHg=-AQAQAAAAAAA='
primary_bucket_list_index = S3kvTest('KvTest fetch root bucket account index')\
    .root_bucket_account_index()
replica_bucket_list_index = S3kvTest('KvTest fetch replica bucket account index')\
    .replica_bucket_account_index()

primary_bucket_metadata_index_oid = "AAAAAAAAAHg=-AgAQAAAAAAA="
replica_bucket_metadata_index_oid = "AAAAAAAAAHg=-BgAQAAAAAAA="
primary_bucket_metadata_index = S3kvTest('KvTest fetch root buket metadata index')\
    .root_bucket_metadata_index()

config = CORTXS3Config()

# ======================================================================================================

# Test Scenario 1
# Test if bucket replica indexes are present.
print("\nvalidate if bucket replica indexes exist.\n")
status, res = CORTXS3IndexApi(config).head(replica_bucket_list_index_oid)
assert status == True
assert isinstance(res, CORTXS3SuccessResponse)

status, res = CORTXS3IndexApi(config).head(replica_bucket_metadata_index_oid)
assert status == True
assert isinstance(res, CORTXS3SuccessResponse)

print("\nHEAD 'replica bucket indexes' validation completed.\n")


# ******************************************************************************************************

# Test Scenario 2
# Test if KV is created in bucket replica indexes at PUT bucket
print("\nvalidate if KV created in replica bucket indexes at PUT bucket\n")
AwsTest('Create Bucket "seagatebucket" using s3-recovery-svc account')\
    .create_bucket("seagatebucket").execute_test().command_is_successful()

# list KV in replica bucket list index
status, res = CORTXS3IndexApi(config).list(replica_bucket_list_index_oid)
assert status == True
assert isinstance(res, CORTXS3ListIndexResponse)

index_content = res.get_index_content()
assert index_content["Index-Id"] == replica_bucket_list_index_oid

replica_KV_list = index_content['Keys']
assert len(replica_KV_list) == 1

# list KV in primary bucket list index
status, res = CORTXS3IndexApi(config).list(primary_bucket_list_index_oid)
assert status == True
assert isinstance(res, CORTXS3ListIndexResponse)

index_content = res.get_index_content()
assert index_content["Index-Id"] == primary_bucket_list_index_oid

primary_KV_list = index_content['Keys']
assert len(primary_KV_list) == 1

# Validate if KV in primary and replica indexes of bucket list are same
for primary_kv, replica_kv in zip(primary_KV_list, replica_KV_list):
    assert primary_kv['Key'] == replica_kv['Key']
    assert primary_kv['Value'] == replica_kv['Value']

# list KV in replica bucket metadata index
status, res = CORTXS3IndexApi(config).list(replica_bucket_metadata_index_oid)
assert status == True
assert isinstance(res, CORTXS3ListIndexResponse)

index_content = res.get_index_content()
replica_KV_list = index_content['Keys']
assert len(replica_KV_list) == 1

# list KV in primary bucket metadata index
status, res = CORTXS3IndexApi(config).list(primary_bucket_metadata_index_oid)
assert status == True
assert isinstance(res, CORTXS3ListIndexResponse)

index_content = res.get_index_content()
primary_KV_list = index_content['Keys']
assert len(primary_KV_list) == 1

# Validate if KV in primary and replica indexes of bucket metadata are same.
for primary_kv, replica_kv in zip(primary_KV_list, replica_KV_list):
    assert primary_kv['Key'] == replica_kv['Key']
    assert primary_kv['Value'] == replica_kv['Value']

# ******************************************************************************************************

# Test Scenario 3
# Test if KV is removed from replica global bucket list index at DELETE bucket
print("\nvalidate if KV deleted from replica bucket indexes at DELETE bucket\n")
AwsTest('Delete Bucket "seagatebucket"').delete_bucket("seagatebucket")\
   .execute_test().command_is_successful()

status, res = CORTXS3IndexApi(config).list(replica_bucket_list_index_oid)
assert status == True
assert isinstance(res, CORTXS3ListIndexResponse)

index_content = res.get_index_content()
assert index_content["Index-Id"] == replica_bucket_list_index_oid
assert index_content["Keys"] == None

status, res = CORTXS3IndexApi(config).list(replica_bucket_metadata_index_oid)
assert status == True
assert isinstance(res, CORTXS3ListIndexResponse)

index_content = res.get_index_content()
assert index_content["Index-Id"] == replica_bucket_metadata_index_oid
assert index_content["Keys"] == None

# ********************** System Tests for s3 recovery tool: dry_run option ********************************************
# Test: KV missing from primary index, but present in replica index
# Step 1: PUT Key-Value in replica index
# Step 2: Run s3 recoverytool with --dry_run option
# Step 3: Validate that the Key-Value is shown on the console as data recovered
# Step 4: Delete the Key-Value from the replica index

st1key = "ST-1-BK"
st1value = '{"account_id":"838334245437",\
     "account_name":"s3-recovery-svc",\
     "create_timestamp":"2020-07-02T05:45:41.000Z",\
     "location_constraint":"us-west-2"}'

# ***************** PUT KV in replica index **********************************
status, res = CORTXS3KVApi(config).put(replica_bucket_list_index_oid, st1key, st1value)
assert status == True
assert isinstance(res, CORTXS3SuccessResponse)

# Run s3 recovery tool
result = S3RecoveryTest(
    "s3recovery --dry_run (KV missing from primary index, but present in replica index)"
    )\
    .s3recovery_dry_run()\
    .execute_test()\
    .command_is_successful()

success_msg = "Data recovered from both indexes for Global bucket index"
result.command_response_should_have(success_msg)

result_stdout_list = (result.status.stdout).split('\n')
assert result_stdout_list[12] != 'Empty'
assert st1key in result_stdout_list[12]
assert '"account_name":"s3-recovery-svc"' in result_stdout_list[12]
assert '"create_timestamp":"2020-07-02T05:45:41.000Z"' in result_stdout_list[12]

# Delete the key-value from replica index
status, res = CORTXS3KVApi(config).delete(replica_bucket_list_index_oid, st1key)
assert status == True
assert isinstance(res, CORTXS3SuccessResponse)

# ************************************************************************************************

# Test: Different Key-Value in primary and replica indexes
# Step 1: PUT Key1-Value1 in primary index
# Step 2: PUT Key2-Value2 in replica index
# Step 2: Run s3 recoverytool with --dry_run option
# Step 3: Validate that both the Key-Values are shown on the console as data recovered
# Step 4: Delete the Key-Values from the both the indexes

primary_index_key = 'my-bucket1'
primary_index_value = '{"account_id":"123456789",\
     "account_name":"s3-recovery-svc",\
     "create_timestamp":"2020-07-14T05:45:41.000Z",\
     "location_constraint":"us-west-2"}'

replica_index_key = 'my-bucket2'
replica_index_value = '{"account_id":"123456789",\
     "account_name":"s3-recovery-svc",\
     "create_timestamp":"2020-08-14T06:45:41.000Z",\
     "location_constraint":"us-east-1"}'

# ***************** PUT the KVs in both the primary and replica indexes *********************************
status, res = CORTXS3KVApi(config)\
    .put(primary_bucket_list_index_oid, primary_index_key, primary_index_value)
assert status == True
assert isinstance(res, CORTXS3SuccessResponse)

status, res = CORTXS3KVApi(config)\
    .put(replica_bucket_list_index_oid, replica_index_key, replica_index_value)
assert status == True
assert isinstance(res, CORTXS3SuccessResponse)

# Run s3 recovery tool
result = S3RecoveryTest(
    "s3recovery --dry_run (Different Key-Value in primary and replica indexes)"
    )\
    .s3recovery_dry_run()\
    .execute_test()\
    .command_is_successful()

success_msg = "Data recovered from both indexes for Global bucket index"
result.command_response_should_have(success_msg)

result_stdout_list = (result.status.stdout).split('\n')
# Example result.status.stdout:
# |-----------------------------------------------------------------------------------------------------------------------------|
#
# Primary index content for Global bucket index
#
# my-bucket2 {"account_id":"123","account_name":"amit-vc","create_timestamp":"2020-06-17T05:45:41.000Z","location_constraint":"us-west-2"}
#
# Replica index content for Global bucket index
#
# my-bucket1 {"account_id":"123","account_name":"amit-vc","create_timestamp":"2020-06-17T05:45:41.000Z","location_constraint":"us-west-2"}
#
# Data recovered from both indexes for Global bucket index
#
# my-bucket2 {"account_id":"123","account_name":"amit-vc","create_timestamp":"2020-06-17T05:45:41.000Z","location_constraint":"us-west-2"}
# my-bucket1 {"account_id":"123","account_name":"amit-vc","create_timestamp":"2020-06-17T05:45:41.000Z","location_constraint":"us-west-2"}
#
# Primary index content for Bucket metadata index
#
# Empty
#
#
# Replica index content for Bucket metadata index
#
# Empty
#
#
# Data recovered from both indexes for Bucket metadata index
#
# Empty
#
# |-----------------------------------------------------------------------------------------------------------------------------|
assert result_stdout_list[3] != 'Empty'

assert primary_index_key in result_stdout_list[3]
assert primary_index_key in result_stdout_list[11]

assert '"create_timestamp":"2020-07-14T05:45:41.000Z"' in result_stdout_list[3]
assert '"create_timestamp":"2020-07-14T05:45:41.000Z"' in result_stdout_list[11]

assert '"location_constraint":"us-west-2"' in result_stdout_list[3]
assert '"location_constraint":"us-west-2"' in result_stdout_list[11]

assert result_stdout_list[7] != 'Empty'

assert replica_index_key in result_stdout_list[7]
assert replica_index_key in result_stdout_list[12]
assert '"create_timestamp":"2020-08-14T06:45:41.000Z"' in result_stdout_list[7]
assert '"create_timestamp":"2020-08-14T06:45:41.000Z"' in result_stdout_list[12]

assert '"location_constraint":"us-east-1"' in result_stdout_list[7]
assert '"location_constraint":"us-east-1"' in result_stdout_list[12]

# Delete the key-values from both primary and replica indexes
status, res = CORTXS3KVApi(config).delete(primary_bucket_list_index_oid, primary_index_key)
assert status == True
assert isinstance(res, CORTXS3SuccessResponse)

status, res = CORTXS3KVApi(config).delete(replica_bucket_list_index_oid, replica_index_key)
assert status == True
assert isinstance(res, CORTXS3SuccessResponse)

# ************************************************************************************************

# Test: Corrupted Value for a key in primary index
# Step 1: PUT corrupted Key1-Value1 in primary index
# Step 2: PUT Key2-Value2 in replica index
# Step 2: Run s3 recoverytool with --dry_run option
# Step 3: Validate that the corrupted Key-Value ins not shown on the console as data recovered
# Step 4: Delete the Key-Values from the both the indexes

primary_index_key = "my-bucket3"
primary_index_corrupted_value = "Corrupted-JSON-value"
S3kvTest('KvTest put corrupted key-value in root bucket account index')\
    .put_keyval(primary_bucket_list_index, primary_index_key, primary_index_corrupted_value)\
    .execute_test()

replica_index_key = "my-bucket4"
replica_index_value = '{"account_id":"123456789",\
     "account_name":"s3-recovery-svc",\
     "create_timestamp":"2020-12-11T06:45:41.000Z",\
     "location_constraint":"mumbai"}'

status, res = CORTXS3KVApi(config)\
    .put(replica_bucket_list_index_oid, replica_index_key, replica_index_value)
assert status == True
assert isinstance(res, CORTXS3SuccessResponse)

# Run s3 recovery tool
result = S3RecoveryTest(
    "s3recovery --dry_run (Corrupted Value for a key in primary index)"
    )\
    .s3recovery_dry_run()\
    .execute_test()\
    .command_is_successful()

success_msg = "Data recovered from both indexes for Global bucket index"
result.command_response_should_have(success_msg)
result_stdout_list = (result.status.stdout).split('\n')

assert replica_index_key in result_stdout_list[12]
assert primary_index_key not in result_stdout_list[12]
assert '"create_timestamp":"2020-12-11T06:45:41.000Z"' in result_stdout_list[12]
assert '"location_constraint":"mumbai"' in result_stdout_list[12]

assert primary_index_key not in result_stdout_list[13]
assert result_stdout_list[13] == ''
assert 'Primary index content for Bucket metadata index' in result_stdout_list[14]

# Delete the key-values from both primary and replica indexes
status, res = CORTXS3KVApi(config).delete(primary_bucket_list_index_oid, primary_index_key)
assert status == True
assert isinstance(res, CORTXS3SuccessResponse)

status, res = CORTXS3KVApi(config).delete(replica_bucket_list_index_oid, replica_index_key)
assert status == True
assert isinstance(res, CORTXS3SuccessResponse)

# ********************** System Tests for s3 recovery tool: recover option ********************************************
key_value_list_with_not_good_values = [
    # corrupted key
    {
        'Key': 'my-bucket5',
        'Value': 'Corrupted-JSON-Value1'
    },
    # key having value same as its counterpart in replica index
    {
        'Key': 'my-bucket6',
        'Value': '{"account_id":"123456789", "account_name":"s3-recovery-svc",\
             "create_timestamp":"2020-11-02T00:01:40.000Z", "location_constraint":"us-east-1"}'
    },
    # key having timestamp less than its counterpart in replica index
    {
        'Key': 'my-bucket7',
        'Value': '{"account_id":"123456789", "account_name":"s3-recovery-svc",\
             "create_timestamp":"2020-01-01T00:01:40.000Z", "location_constraint":"mumbai"}'
    },
    # this key would be missing from replica index
    {
        'Key': 'my-bucket8',
        'Value': '{"account_id":"123456789", "account_name":"s3-recovery-svc",\
             "create_timestamp":"2020-01-02T00:01:40.000Z", "location_constraint":"us-east-1"}'
    },
    # this key would be missing from primary index
    {
        'Key': 'my-bucket9',
        'Value': '{"account_id":"123456789", "account_name":"s3-recovery-svc",\
             "create_timestamp":"2020-01-02T00:10:40.000Z", "location_constraint":"us-west-2"}' 
    }
]
key_value_list_with_all_good_values = [
    # this key has good value, as compared to its counterpart in primary index
    {
        'Key': 'my-bucket5',
        'Value': '{"account_id":"123456789", "account_name":"s3-recovery-svc",\
             "create_timestamp":"2020-01-01T06:01:40.000Z", "location_constraint":"EU"}'
    },
    # this key has same value as its counterpart in primary index
    {
        'Key': 'my-bucket6',
        'Value': '{"account_id":"123456789", "account_name":"s3-recovery-svc",\
             "create_timestamp":"2020-11-02T00:01:40.000Z", "location_constraint":"us-east-1"}'
    },
    # this key has timestamp greater than its counterpart in primary index
    {
        'Key': 'my-bucket7',
        'Value': '{"account_id":"123456789", "account_name":"s3-recovery-svc",\
             "create_timestamp":"2020-06-01T00:01:40.000Z", "location_constraint":"mumbai"}'
    },
    # this key would be missing from replica index
    {
        'Key': 'my-bucket8',
        'Value': '{"account_id":"123456789", "account_name":"s3-recovery-svc",\
             "create_timestamp":"2020-01-02T00:01:40.000Z", "location_constraint":"us-east-1"}'
    },
    # this key would be missing from primary index
    {
        'Key': 'my-bucket9',
        'Value': '{"account_id":"123456789", "account_name":"s3-recovery-svc",\
             "create_timestamp":"2020-01-02T00:10:40.000Z", "location_constraint":"us-west-2"}'
    }
]

def validate_if_same_kvs(index1_kv_list, index2_kv_list):
    """
    check if both index1 and index2 have same key-value pairs or not.
    """
    assert len(index1_kv_list) == len(index2_kv_list)
    for KV in index1_kv_list:
        assert KV in index2_kv_list 

def put_kvs_in_indexes(index_with_corrupt_kvs,
    index_with_correct_kvs):
    """
    put key-values in the given indexes
    index_with_corrupt_kvs: index id which will have corrupt key-values in it
    index_with_correct_kvs: index id which will have all key-values as good
    """
    # PUT KVs in first index id: index_with_corrupt_kvs
    for KV in key_value_list_with_not_good_values:
        # 'my-bucket5' has corrupted value, use s3cloviscli to PUT it
        if KV['Key'] == 'my-bucket5':
            if index_with_corrupt_kvs == primary_bucket_list_index_oid:
                put_in_index = primary_bucket_list_index
            else:
                put_in_index = replica_bucket_list_index
            S3kvTest('KvTest put corrupted KV in index_with_corrupt_kvs')\
            .put_keyval(put_in_index, KV['Key'], KV['Value'])\
            .execute_test().command_is_successful()
        else:
            if KV['Key'] == "my-bucket9":
                # skip this Key, to check if "recover" put this key in the index
                continue
            status, res = CORTXS3KVApi(config)\
                .put(index_with_corrupt_kvs, KV['Key'], KV['Value'])
            assert status == True
            assert isinstance(res, CORTXS3SuccessResponse)

    # PUT KVs in second index id: index_with_correct_kvs
    for KV in key_value_list_with_all_good_values:
        # we need to skip this key to validate if "recover" restores this key in the index
        if KV['Key'] == "my-bucket8":
            continue
        status, res = CORTXS3KVApi(config)\
            .put(index_with_correct_kvs, KV['Key'], KV['Value'])
        assert status == True
        assert isinstance(res, CORTXS3SuccessResponse)

    # PUT KVs in global bucket metadata index
    # TODO: As of now we are adding correct KVs in global bucket metadata index
    # as it does not have replcia index. Fix this once replica index is added
    # for global bucket metadata index
    for KV in key_value_list_with_all_good_values:
        key = r'123456789/' + KV['Key']
        status, res = CORTXS3KVApi(config)\
            .put(primary_bucket_metadata_index_oid, key, KV['Value'])
        assert status == True
        assert isinstance(res, CORTXS3SuccessResponse)

# ***************************************************************************************************
# Test: recover corrupted, old and missing key-values in primary index from replica index 
# Step 1: PUT 4 Key-Values (K1V1, K2V2, K3V3, K4V4) in primary index, including 1 corrupted key-value.
# Step 2: Put 4 Keys but different Values (K1V'1, K2V2, K3V'3, K5V5) in replica index.
# Step 3: Put 5 keys in global bucket metadata index. (Global bucket metadata index do not have replica index
# as of now, so we need to put all 5 keys with good values to the index) 
# Step 4: Run s3 recoverytool with --recover option.
# Step 5: Validate that both primary and replica index have 5 key-values,
# with no key having corrupted value, both still having the same K2V2 and K3 has V'3 value in both indexes.
# Step 6: Delete the Key-Values from the primary and replica indexes.

# PUT KVs in the primary and replica indexes of global bucket list
# primary index would have corrupt and old KVs, replica index would have
# correct and latest KVS
put_kvs_in_indexes(primary_bucket_list_index_oid, replica_bucket_list_index_oid)

# Validate that primary and replica indexes had 4 keys before running the s3recoverytool
status, res = CORTXS3IndexApi(config).list(primary_bucket_list_index_oid)
assert status == True
assert isinstance(res, CORTXS3ListIndexResponse)

index_content = res.get_index_content()
key_value_primary_index_before_recovery_list = index_content["Keys"]
assert index_content["Index-Id"] == primary_bucket_list_index_oid
assert len(key_value_primary_index_before_recovery_list) == 4

status, res = CORTXS3IndexApi(config).list(replica_bucket_list_index_oid)
assert status == True
assert isinstance(res, CORTXS3ListIndexResponse)

index_content = res.get_index_content()
key_value_replica_index_before_recovery_list = index_content["Keys"]
assert index_content["Index-Id"] == replica_bucket_list_index_oid
assert len(key_value_replica_index_before_recovery_list) == 4

# Run s3 recovery tool
result = S3RecoveryTest("s3recovery --recover (corrupted and missing KVs)")\
    .s3recovery_recover()\
    .execute_test()\
    .command_is_successful()

# List Primary Index
status, res = CORTXS3IndexApi(config).list(primary_bucket_list_index_oid)
assert status == True
assert isinstance(res, CORTXS3ListIndexResponse)

index_content = res.get_index_content()
assert index_content["Index-Id"] == primary_bucket_list_index_oid
actual_primary_index_key_value_list = index_content["Keys"]

# List replica Index
status, res = CORTXS3IndexApi(config).list(replica_bucket_list_index_oid)
assert status == True
assert isinstance(res, CORTXS3ListIndexResponse)

index_content = res.get_index_content()
assert index_content["Index-Id"] == replica_bucket_list_index_oid
actual_replica_index_key_value_list = index_content["Keys"]

# validate if both primary and replica index have same keys and values
validate_if_same_kvs(actual_primary_index_key_value_list,
    actual_replica_index_key_value_list)

# Delete the key-values from both primary and replica indexes
for KV in key_value_list_with_not_good_values:
    status, res = CORTXS3KVApi(config).delete(primary_bucket_list_index_oid, KV['Key'])
    assert status == True
    assert isinstance(res, CORTXS3SuccessResponse)

    status, res = CORTXS3KVApi(config).delete(replica_bucket_list_index_oid, KV['Key'])
    assert status == True
    assert isinstance(res, CORTXS3SuccessResponse)

    metadata_index_key = r'123456789/' + KV['Key']
    status, res = CORTXS3KVApi(config)\
        .delete(primary_bucket_metadata_index_oid, metadata_index_key)
    assert status == True
    assert isinstance(res, CORTXS3SuccessResponse)

# ***************************************************************************************************
# Test: recover corrupted, old and missing key-values in replica index from primary index 
# Step 1: PUT 4 Key-Values (K1V1, K2V2, K3V3, K4V4) in replica index, including 1 corrupted key-value.
# Step 2: Put 4 Keys but different Values (K1V'1, K2V2, K3V'3, K5V5) in primary index.
# Step 3: Put 5 keys in global bucket metadata index. (Global bucket metadata index do not have replica index
# as of now, so we need to put all 5 keys with good values to the index) 
# Step 4: Run s3 recoverytool with --recover option.
# Step 5: Validate that both primary and replica index have 5 key-values,
# with no key having corrupted value, both still having the same K2V2 and K3 has V'3 value in both indexes.
# Step 6: Delete the Key-Values from the primary and replica indexes.

# PUT KVs in the replica and primary indexes of global bucket list
# replica index would have corrupt and old KVs, primary index would have
# correct and latest KVS
put_kvs_in_indexes(replica_bucket_list_index_oid, primary_bucket_list_index_oid)

# Validate that primary and replica indexes had 4 keys before running the s3recoverytool
status, res = CORTXS3IndexApi(config).list(primary_bucket_list_index_oid)
assert status == True
assert isinstance(res, CORTXS3ListIndexResponse)

index_content = res.get_index_content()
key_value_primary_index_before_recovery_list = index_content["Keys"]
assert index_content["Index-Id"] == primary_bucket_list_index_oid
assert len(key_value_primary_index_before_recovery_list) == 4

status, res = CORTXS3IndexApi(config).list(replica_bucket_list_index_oid)
assert status == True
assert isinstance(res, CORTXS3ListIndexResponse)

index_content = res.get_index_content()
key_value_replica_index_before_recovery_list = index_content["Keys"]
assert index_content["Index-Id"] == replica_bucket_list_index_oid
assert len(key_value_replica_index_before_recovery_list) == 4

# Run s3 recovery tool
result = S3RecoveryTest("s3recovery --recover (corrupted and missing KVs)")\
    .s3recovery_recover()\
    .execute_test()\
    .command_is_successful()

# List Primary Index
status, res = CORTXS3IndexApi(config).list(primary_bucket_list_index_oid)
assert status == True
assert isinstance(res, CORTXS3ListIndexResponse)

index_content = res.get_index_content()
assert index_content["Index-Id"] == primary_bucket_list_index_oid
actual_primary_index_key_value_list = index_content["Keys"]

# List replica Index
status, res = CORTXS3IndexApi(config).list(replica_bucket_list_index_oid)
assert status == True
assert isinstance(res, CORTXS3ListIndexResponse)

index_content = res.get_index_content()
assert index_content["Index-Id"] == replica_bucket_list_index_oid
actual_replica_index_key_value_list = index_content["Keys"]

# validate if both primary and replica index have same keys and values
validate_if_same_kvs(actual_primary_index_key_value_list,
    actual_replica_index_key_value_list)

# Delete the key-values from both primary and replica indexes
for KV in key_value_list_with_not_good_values:
    status, res = CORTXS3KVApi(config).delete(primary_bucket_list_index_oid, KV['Key'])
    assert status == True
    assert isinstance(res, CORTXS3SuccessResponse)

    status, res = CORTXS3KVApi(config).delete(replica_bucket_list_index_oid, KV['Key'])
    assert status == True
    assert isinstance(res, CORTXS3SuccessResponse)

    metadata_index_key = r'123456789/' + KV['Key']
    status, res = CORTXS3KVApi(config)\
        .delete(primary_bucket_metadata_index_oid, metadata_index_key)
    assert status == True
    assert isinstance(res, CORTXS3SuccessResponse)

#-------------------------------------------------------------------------------------------------------

# Test: s3recoverytool consistency check
# Step 1: PUT 3 Key-Values (KV1, KV2, KV3) in primary index and replica index of global bucket list.
# Step 2: Put 2 Key-Values (KV1, KV2) in primary and replica index of global bucket metadata.
# Step 3: Run s3 recoverytool with --recover option.
# Step 4: Validate that both indexes (bucket list and bucket metadata) have 2 KVs KV1 and KV2.
# Step 5: Delete the Key-Values from the primary and replica indexes.

global_bucket_list_index_key_value_list = [
    {
        'Key': 'my-bucket10',
        'Value': '{"account_id":"123456789", "account_name":"s3-recovery-svc",\
             "create_timestamp":"2020-01-01T06:01:40.000Z", "location_constraint":"EU"}'
    },
    {
        'Key': 'my-bucket11',
        'Value': '{"account_id":"123456789", "account_name":"s3-recovery-svc",\
             "create_timestamp":"2020-01-01T06:05:40.000Z", "location_constraint":"sa-east-1"}'
    },
    {
        'Key': 'my-bucket12',
        'Value': '{"account_id":"123456789", "account_name":"s3-recovery-svc",\
             "create_timestamp":"2020-06-01T00:01:40.000Z", "location_constraint":"mumbai"}'
    }
]
global_bucket_metadata_index_key_value_list = [
    {
        'Key': 'my-bucket10',
        'Value': '{"account_id":"123456789", "account_name":"s3-recovery-svc",\
             "create_timestamp":"2020-01-01T06:01:40.000Z", "location_constraint":"EU"}'
    },
    {
        'Key': 'my-bucket11',
        'Value': '{"account_id":"123456789", "account_name":"s3-recovery-svc",\
             "create_timestamp":"2020-01-01T06:05:40.000Z", "location_constraint":"sa-east-1"}'
    }
]

# PUT KV in global bucket list index, primary and replica both
for KV in global_bucket_list_index_key_value_list:
    status, res = CORTXS3KVApi(config)\
        .put(primary_bucket_list_index_oid, KV['Key'], KV['Value'])
    assert status == True
    assert isinstance(res, CORTXS3SuccessResponse)

    status, res = CORTXS3KVApi(config)\
        .put(replica_bucket_list_index_oid, KV['Key'], KV['Value'])
    assert status == True
    assert isinstance(res, CORTXS3SuccessResponse)

# PUT KV in global bucket metadata index
for KV in global_bucket_metadata_index_key_value_list:
    key = r'123456789/' + KV['Key']
    status, res = CORTXS3KVApi(config)\
        .put(primary_bucket_metadata_index_oid, key, KV['Value'])
    assert status == True
    assert isinstance(res, CORTXS3SuccessResponse)

# Validate 3 KVs in global bucket list index
status, res = CORTXS3IndexApi(config).list(primary_bucket_list_index_oid)
assert status == True
assert isinstance(res, CORTXS3ListIndexResponse)

index_content = res.get_index_content()
key_value_primary_index_before_recovery_list = index_content["Keys"]
assert index_content["Index-Id"] == primary_bucket_list_index_oid
assert len(key_value_primary_index_before_recovery_list) == 3

# Run s3 recovery tool
result = S3RecoveryTest("s3recovery --recover (consistency check)")\
    .s3recovery_recover()\
    .execute_test()\
    .command_is_successful()

# Validate 2 KVs in global bucket list index after doing 's3recovery --recover' 
status, res = CORTXS3IndexApi(config).list(primary_bucket_list_index_oid)
assert status == True
assert isinstance(res, CORTXS3ListIndexResponse)

index_content = res.get_index_content()
key_value_primary_index_before_recovery_list = index_content["Keys"]
assert index_content["Index-Id"] == primary_bucket_list_index_oid
assert len(key_value_primary_index_before_recovery_list) == 2

# Validate 2 KVs in global bucket metadata index after doing 's3recovery --recover' 
status, res = CORTXS3IndexApi(config).list(primary_bucket_metadata_index_oid)
assert status == True
assert isinstance(res, CORTXS3ListIndexResponse)

index_content = res.get_index_content()
key_value_primary_index_before_recovery_list = index_content["Keys"]
assert index_content["Index-Id"] == primary_bucket_metadata_index_oid
assert len(key_value_primary_index_before_recovery_list) == 2
 
# Delete the key-values from both primary and replica indexes
for KV in global_bucket_list_index_key_value_list:
    status, res = CORTXS3KVApi(config).delete(primary_bucket_list_index_oid, KV['Key'])
    assert status == True
    assert isinstance(res, CORTXS3SuccessResponse)

    status, res = CORTXS3KVApi(config).delete(replica_bucket_list_index_oid, KV['Key'])
    assert status == True
    assert isinstance(res, CORTXS3SuccessResponse)

for KV in global_bucket_metadata_index_key_value_list:
    metadata_index_key = r'123456789/' + KV['Key']
    status, res = CORTXS3KVApi(config)\
        .delete(primary_bucket_metadata_index_oid, metadata_index_key)
    assert status == True
    assert isinstance(res, CORTXS3SuccessResponse)

# ================================================= CLEANUP =============================================

# ************ Delete Account*******************************
test_msg = "Delete account s3-recovery-svc"
account_args = {'AccountName': 's3-recovery-svc',\
               'Email': 's3-recovery-svc@seagate.com', 'force': True}
AuthTest(test_msg).delete_account(**account_args).execute_test()\
    .command_response_should_have("Account deleted successfully")

# Restore s3backgrounddelete config file.
restore_configuration()
