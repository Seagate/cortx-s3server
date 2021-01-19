#!/usr/bin/python3.6
#
# Copyright (c) 2020 Seagate Technology LLC and/or its Affiliates
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# For any questions about this software or licensing,
# please email opensource@seagate.com or cortx-questions@seagate.com.
#



import sys
import os
import yaml
import shutil
import re
import json
from framework import Config
from ldap_setup import LdapInfo
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
S3ClientConfig.ldappasswd = LdapInfo.get_ldap_admin_pwd()

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
            config['s3_recovery']['recovery_account_access_key'] = access_key_value
            config['s3_recovery']['recovery_account_secret_key'] = secret_key_value
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
primary_bucket_metadata_index = S3kvTest('KvTest fetch root bucket metadata index')\
    .root_bucket_metadata_index()
replica_bucket_metadata_index = S3kvTest('KvTest fetch replica bucket metadata index')\
    .replica_bucket_metadata_index()

config = CORTXS3Config(s3recovery_flag = True)

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
# Test: KV missing from primary index, but present in replica index of both bucket list and metadata indexes
# Step 1: PUT Key-Value in replica index of both bucket list and metadata indexes
# Step 2: Run s3 recoverytool with --dry_run option
# Step 3: Validate that the Key-Value is shown on the console as data recovered for both indexes
# Step 4: Delete the Key-Value from the replica index of both bucket list and metadata indexes

st1key = "ST-1-BK"
st1value_list_index = '{"account_id":"838334245437",\
     "account_name":"s3-recovery-svc",\
     "create_timestamp":"2020-07-02T05:45:41.000Z",\
     "location_constraint":"us-west-2"}'

st1value_metadata_index = '{"ACL":"", "Bucket-Name":"ST-1-BK", "Policy":"",\
     "System-Defined": { "Date":"2020-07-20T09:08:11.000Z", "LocationConstraint":"us-west-2",\
     "Owner-Account":"temp-acc", "Owner-Account-id":"286991820398", "Owner-User":"root",\
     "Owner-User-id":"HzG2EFe-RaWSyeecGAKafQ" },\
     "create_timestamp":"2020-07-20T09:08:11.000Z",\
     "motr_multipart_index_oid":"lwnlAAAAAHg=-FAAAAAAAlko=",\
     "motr_object_list_index_oid":"lwnlAAAAAHg=-EwAAAAAAlko=",\
     "motr_objects_version_list_index_oid":"lwnlAAAAAHg=-FQAAAAAAlko=" }'

# PUT KV in replica index of global bucket list index
status, res = CORTXS3KVApi(config)\
    .put(replica_bucket_list_index_oid, st1key, st1value_list_index)
assert status == True
assert isinstance(res, CORTXS3SuccessResponse)

# PUT KV in replica index of global bucket metadata index
status, res = CORTXS3KVApi(config)\
    .put(replica_bucket_metadata_index_oid, r'838334245437/' + st1key, st1value_metadata_index)
assert status == True
assert isinstance(res, CORTXS3SuccessResponse)

# Run s3 recovery tool
result = S3RecoveryTest(
    "s3recovery --dry_run (KV missing from primary index, but present in replica index)"
    )\
    .s3recovery_dry_run()\
    .execute_test()\
    .command_is_successful()

success_msg_list_index = "Data recovered from both indexes for Global bucket index"
success_msg_metadata_index = "Data recovered from both indexes for Bucket metadata index"

result.command_response_should_have(success_msg_list_index)
result.command_response_should_have(success_msg_metadata_index)

result_stdout_list = (result.status.stdout).split('\n')

assert result_stdout_list[10] != 'Empty'
assert result_stdout_list[15] != 'Empty'
assert st1key in result_stdout_list[15]
assert '"account_name":"s3-recovery-svc"' in result_stdout_list[15]
assert '"create_timestamp":"2020-07-02T05:45:41.000Z"' in result_stdout_list[15]

assert result_stdout_list[26] != 'Empty'
assert result_stdout_list[31] != 'Empty'
assert r'838334245437/' + st1key in result_stdout_list[31]
assert 'ACL' in result_stdout_list[31]
assert 'System-Defined' in result_stdout_list[31]

# Delete the key-value from replica index
status, res = CORTXS3KVApi(config).delete(replica_bucket_list_index_oid, st1key)
assert status == True
assert isinstance(res, CORTXS3SuccessResponse)

status, res = CORTXS3KVApi(config).delete(replica_bucket_metadata_index_oid, r'838334245437/' + st1key)
assert status == True
assert isinstance(res, CORTXS3SuccessResponse)

# ********************** System Tests for s3 recovery tool: dry_run option ********************************************
# Test: Different Key-Value in primary and replica indexes
# Step 1: PUT Key1-Value1 in primary index of both bucket list and metadata
# Step 2: PUT Key2-Value2 in replica index of both bucket list and metadata
# Step 2: Run s3 recoverytool with --dry_run option
# Step 3: Validate that both the Key-Values are shown on the console as data recovered
# Step 4: Delete the Key-Values from all indexes

primary_index_key = 'my-bucket1'
primary_list_index_value = '{"account_id":"123456789",\
     "account_name":"s3-recovery-svc",\
     "create_timestamp":"2020-07-14T05:45:41.000Z",\
     "location_constraint":"us-west-2"}'
primary_metadata_index_value = '{"ACL":"", "Bucket-Name":"my-bucket1", "Policy":"",\
     "System-Defined": { "Date":"2020-07-14T05:45:41.000Z", "LocationConstraint":"us-west-2",\
     "Owner-Account":"s3-recovery-svc", "Owner-Account-id":"123456789", "Owner-User":"root",\
     "Owner-User-id":"HzG2EFe-RaWSyeecGAKafQ" }, "create_timestamp":"2020-07-14T05:45:41.000Z",\
     "motr_multipart_index_oid":"lwnlAAAAAHg=-FAAAAAAAlko=",\
     "motr_object_list_index_oid":"lwnlAAAAAHg=-EwAAAAAAlko=",\
     "motr_objects_version_list_index_oid":"lwnlAAAAAHg=-FQAAAAAAlko="}'

replica_index_key = 'my-bucket2'
replica_list_index_value = '{"account_id":"123456789",\
     "account_name":"s3-recovery-svc",\
     "create_timestamp":"2020-08-14T06:45:41.000Z",\
     "location_constraint":"us-east-1"}'
replica_metadata_index_value = '{"ACL":"", "Bucket-Name":"my-bucket2", "Policy":"",\
     "System-Defined": { "Date":"2020-08-14T06:45:41.000Z", "LocationConstraint":"us-east-1",\
     "Owner-Account":"s3-recovery-svc", "Owner-Account-id":"123456789", "Owner-User":"root",\
     "Owner-User-id":"HzG2EFe-RaWSyeecGAKafQ" }, "create_timestamp":"2020-08-14T06:45:41.000Z",\
     "motr_multipart_index_oid":"lwnlAAAAAHg=-FAAAAAAAlko=",\
     "motr_object_list_index_oid":"lwnlAAAAAHg=-EwAAAAAAlko=",\
     "motr_objects_version_list_index_oid":"lwnlAAAAAHg=-FQAAAAAAlko="}'

# PUT the KVs in primary and replica indexes of bucket list index
status, res = CORTXS3KVApi(config)\
    .put(primary_bucket_list_index_oid, primary_index_key, primary_list_index_value)
assert status == True
assert isinstance(res, CORTXS3SuccessResponse)

status, res = CORTXS3KVApi(config)\
    .put(replica_bucket_list_index_oid, replica_index_key, replica_list_index_value)
assert status == True
assert isinstance(res, CORTXS3SuccessResponse)

# PUT the KVs in primary and replica indexes of bucket metadata index
status, res = CORTXS3KVApi(config)\
    .put(primary_bucket_metadata_index_oid, r'123456789/' + primary_index_key, primary_metadata_index_value)
assert status == True
assert isinstance(res, CORTXS3SuccessResponse)

status, res = CORTXS3KVApi(config)\
    .put(replica_bucket_metadata_index_oid, r'123456789/' + replica_index_key, replica_metadata_index_value)
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
# s3recovery --dry_run
#
# Primary index content for Global bucket index
#
# my-bucket1 {"account_id":"123456789", "account_name":"s3-recovery-svc", "create_timestamp":"2020-07-14T05:45:41.000Z", "location_constraint":"us-west-2"}
#
# Replica index content for Global bucket index
#
# my-bucket2 {"account_id":"123456789", "account_name":"s3-recovery-svc", "create_timestamp":"2020-08-14T06:45:41.000Z", "location_constraint":"us-east-1"}
#
# Data recovered from both indexes for Global bucket index
#
# my-bucket1 {"account_id":"123456789", "account_name":"s3-recovery-svc", "create_timestamp":"2020-07-14T05:45:41.000Z", "location_constraint":"us-west-2"}
# my-bucket2 {"account_id":"123456789", "account_name":"s3-recovery-svc", "create_timestamp":"2020-08-14T06:45:41.000Z", "location_constraint":"us-east-1"}
#
# Primary index content for Bucket metadata index
#
# 123456789/my-bucket1 {"ACL":"", "Bucket-Name":"my-bucket1", "Policy":"", "System-Defined": { "Date":"2020-07-14T05:45:41.000Z", "LocationConstraint":"us-west-2", "Owner-Account":"s3-recovery-svc", "Owner-Account-id":"123456789", "Owner-User":"root", "Owner-User-id":"HzG2EFe-RaWSyeecGAKafQ" }, "create_timestamp":"2020-07-14T05:45:41.000Z", "motr_multipart_index_oid":"lwnlAAAAAHg=-FAAAAAAAlko=", "motr_object_list_index_oid":"lwnlAAAAAHg=-EwAAAAAAlko=", "motr_objects_version_list_index_oid":"lwnlAAAAAHg=-FQAAAAAAlko="}
#
# Replica index content for Bucket metadata index
#
# 123456789/my-bucket2 {"ACL":"", "Bucket-Name":"my-bucket1", "Policy":"", "System-Defined": { "Date":"2020-08-14T06:45:41.000Z", "LocationConstraint":"us-east-1", "Owner-Account":"s3-recovery-svc", "Owner-Account-id":"123456789", "Owner-User":"root", "Owner-User-id":"HzG2EFe-RaWSyeecGAKafQ" }, "create_timestamp":"2020-08-14T06:45:41.000Z", "motr_multipart_index_oid":"lwnlAAAAAHg=-FAAAAAAAlko=", "motr_object_list_index_oid":"lwnlAAAAAHg=-EwAAAAAAlko=", "motr_objects_version_list_index_oid":"lwnlAAAAAHg=-FQAAAAAAlko="}
#
# Data recovered from both indexes for Bucket metadata index
#
# 123456789/my-bucket1 {"ACL":"", "Bucket-Name":"my-bucket1", "Policy":"", "System-Defined": { "Date":"2020-07-14T05:45:41.000Z", "LocationConstraint":"us-west-2", "Owner-Account":"s3-recovery-svc", "Owner-Account-id":"123456789", "Owner-User":"root", "Owner-User-id":"HzG2EFe-RaWSyeecGAKafQ" }, "create_timestamp":"2020-07-14T05:45:41.000Z", "motr_multipart_index_oid":"lwnlAAAAAHg=-FAAAAAAAlko=", "motr_object_list_index_oid":"lwnlAAAAAHg=-EwAAAAAAlko=", "motr_objects_version_list_index_oid":"lwnlAAAAAHg=-FQAAAAAAlko="}
# 123456789/my-bucket2 {"ACL":"", "Bucket-Name":"my-bucket1", "Policy":"", "System-Defined": { "Date":"2020-08-14T06:45:41.000Z", "LocationConstraint":"us-east-1", "Owner-Account":"s3-recovery-svc", "Owner-Account-id":"123456789", "Owner-User":"root", "Owner-User-id":"HzG2EFe-RaWSyeecGAKafQ" }, "create_timestamp":"2020-08-14T06:45:41.000Z", "motr_multipart_index_oid":"lwnlAAAAAHg=-FAAAAAAAlko=", "motr_object_list_index_oid":"lwnlAAAAAHg=-EwAAAAAAlko=", "motr_objects_version_list_index_oid":"lwnlAAAAAHg=-FQAAAAAAlko="}
# |-----------------------------------------------------------------------------------------------------------------------------|
assert result_stdout_list[4] != 'Empty'

assert primary_index_key in result_stdout_list[4]
assert primary_index_key in result_stdout_list[14]

assert '"create_timestamp":"2020-07-14T05:45:41.000Z"' in result_stdout_list[4]
assert '"create_timestamp":"2020-07-14T05:45:41.000Z"' in result_stdout_list[14]

assert '"location_constraint":"us-west-2"' in result_stdout_list[4]
assert '"location_constraint":"us-west-2"' in result_stdout_list[14]

assert result_stdout_list[9] != 'Empty'

assert replica_index_key in result_stdout_list[9]
assert replica_index_key in result_stdout_list[15]
assert '"create_timestamp":"2020-08-14T06:45:41.000Z"' in result_stdout_list[9]
assert '"create_timestamp":"2020-08-14T06:45:41.000Z"' in result_stdout_list[15]

assert '"location_constraint":"us-east-1"' in result_stdout_list[9]
assert '"location_constraint":"us-east-1"' in result_stdout_list[15]

# Delete the key-values from both primary and replica indexes
status, res = CORTXS3KVApi(config).delete(primary_bucket_list_index_oid, primary_index_key)
assert status == True
assert isinstance(res, CORTXS3SuccessResponse)

status, res = CORTXS3KVApi(config).delete(replica_bucket_list_index_oid, replica_index_key)
assert status == True
assert isinstance(res, CORTXS3SuccessResponse)

status, res = CORTXS3KVApi(config).delete(primary_bucket_metadata_index_oid, r'123456789/' + primary_index_key)
assert status == True
assert isinstance(res, CORTXS3SuccessResponse)

status, res = CORTXS3KVApi(config).delete(replica_bucket_metadata_index_oid, r'123456789/' + replica_index_key)
assert status == True
assert isinstance(res, CORTXS3SuccessResponse)

# ********************** System Tests for s3 recovery tool: dry_run option ********************************************
# Test: Corrupted Value for a key in primary index
# Step 1: PUT corrupted Key1-Value1 in primary bucket list index
# Step 2: PUT correct Key2-Value2 in replica bucket list index
# Step 3: PUT correct Key3-Value3 in primary bucket metadata index
# Step 4: PUT corrupted Key3-Value3' in replica bucket metadata index
# Step 5: Run s3 recoverytool with --dry_run option
# Step 6: Validate that the corrupted Key1-Value1 is not shown on the console\
# as data recovered for global bucket list index, and Key3 has correct value (value3, not value3')\
#  shown as data recovered for bucket metadata index.
# Step 7: Delete the Key-Values from the both the indexes.

primary_list_index_key = "my-bucket3"
primary_list_index_corrupted_value = "Corrupted-JSON-value1"
S3kvTest('KvTest put corrupted key-value in root bucket account index')\
    .put_keyval(primary_bucket_list_index, primary_list_index_key, primary_list_index_corrupted_value)\
    .execute_test()

replica_list_index_key = "my-bucket4"
replica_list_index_value = '{"account_id":"123456789",\
     "account_name":"s3-recovery-svc",\
     "create_timestamp":"2020-12-11T06:45:41.000Z",\
     "location_constraint":"mumbai"}'

status, res = CORTXS3KVApi(config)\
    .put(replica_bucket_list_index_oid, replica_list_index_key, replica_list_index_value)
assert status == True
assert isinstance(res, CORTXS3SuccessResponse)

primary_metadata_index_key = "my-bucket5"
primary_metadata_index_value = '{"ACL":"", "Bucket-Name":"my-bucket5", "Policy":"",\
     "System-Defined": { "Date":"2020-08-14T06:45:41.000Z", "LocationConstraint":"us-east-1",\
     "Owner-Account":"s3-recovery-svc", "Owner-Account-id":"123456789", "Owner-User":"root",\
     "Owner-User-id":"HzG2EFe-RaWSyeecGAKafQ" }, "create_timestamp":"2020-08-14T06:45:41.000Z",\
     "motr_multipart_index_oid":"lwnlAAAAAHg=-FAAAAAAAlko=",\
     "motr_object_list_index_oid":"lwnlAAAAAHg=-EwAAAAAAlko=",\
     "motr_objects_version_list_index_oid":"lwnlAAAAAHg=-FQAAAAAAlko="}'

status, res = CORTXS3KVApi(config)\
    .put(primary_bucket_metadata_index_oid, r'123456789/' + primary_metadata_index_key,\
         primary_metadata_index_value)
assert status == True
assert isinstance(res, CORTXS3SuccessResponse)

replica_metadata_index_corrupted_value = "Corrupted-JSON-value2"
S3kvTest('KvTest put corrupted key-value in replica bucket metadata index')\
    .put_keyval(replica_bucket_metadata_index, r'123456789/' + primary_metadata_index_key,\
         replica_metadata_index_corrupted_value)\
    .execute_test()

# Run s3 recovery tool
result = S3RecoveryTest(
    "s3recovery --dry_run (Corrupted Value for a key in primary index)"
    )\
    .s3recovery_dry_run()\
    .execute_test()\
    .command_is_successful()

success_msg_list_index = "Data recovered from both indexes for Global bucket index"
result.command_response_should_have(success_msg_list_index)

success_msg_metadata_index = "Data recovered from both indexes for Bucket metadata index"
result.command_response_should_have(success_msg_metadata_index)

result_stdout_list = (result.status.stdout).split('\n')

# Validate the dry_run output
assert replica_list_index_key in result_stdout_list[15]
assert primary_list_index_key not in result_stdout_list[16]
assert replica_list_index_value in result_stdout_list[15]

assert primary_index_key not in result_stdout_list[16]
assert 'Primary index content for Bucket metadata index' in result_stdout_list[17]

# Delete the key-values from both primary and replica indexes
status, res = CORTXS3KVApi(config).delete(primary_bucket_list_index_oid, primary_list_index_key)
assert status == True
assert isinstance(res, CORTXS3SuccessResponse)

status, res = CORTXS3KVApi(config).delete(replica_bucket_list_index_oid, replica_list_index_key)
assert status == True
assert isinstance(res, CORTXS3SuccessResponse)

status, res = CORTXS3KVApi(config)\
    .delete(primary_bucket_metadata_index_oid, r'123456789/' + primary_metadata_index_key)
assert status == True
assert isinstance(res, CORTXS3SuccessResponse)

status, res = CORTXS3KVApi(config).delete(replica_bucket_metadata_index_oid, r'123456789/' + primary_metadata_index_key)
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
    },
    # this key will have missing epoch entry
    {
        'Key': 'my-bucket10',
        'Value': '{"account_id":"123456789", "account_name":"s3-recovery-svc","location_constraint":"us-west-2"}'
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
        # 'my-bucket5' has corrupted value, use s3motrcli to PUT it
        if KV['Key'] == 'my-bucket5':
            if index_with_corrupt_kvs == primary_bucket_list_index_oid:
                put_in_index = primary_bucket_list_index
                key = KV['Key']
            elif index_with_corrupt_kvs == primary_bucket_metadata_index_oid:
                put_in_index = primary_bucket_metadata_index
                key = r'123456789/' + KV['Key']
            elif index_with_corrupt_kvs == replica_bucket_list_index_oid:
                put_in_index = replica_bucket_list_index
                key = KV['Key']
            elif index_with_corrupt_kvs == replica_bucket_metadata_index_oid:
                put_in_index = replica_bucket_metadata_index
                key = r'123456789/' + KV['Key']
            S3kvTest('KvTest put corrupted KV in index_with_corrupt_kvs')\
            .put_keyval(put_in_index, key, KV['Value'])\
            .execute_test().command_is_successful()
        else:
            if KV['Key'] == "my-bucket9":
                # skip this Key, to check if "recover" put this key in the index
                continue
            if index_with_corrupt_kvs == primary_bucket_metadata_index_oid or\
                index_with_corrupt_kvs == replica_bucket_metadata_index_oid:
                key = r'123456789/' + KV['Key']
            else:
                key = KV['Key']
            status, res = CORTXS3KVApi(config)\
                .put(index_with_corrupt_kvs, key, KV['Value'])
            assert status == True
            assert isinstance(res, CORTXS3SuccessResponse)

    # PUT KVs in second index id: index_with_correct_kvs
    for KV in key_value_list_with_all_good_values:
        # we need to skip this key to validate if "recover" restores this key in the index
        if KV['Key'] == "my-bucket8":
            continue
        if index_with_corrupt_kvs == primary_bucket_metadata_index_oid or\
            index_with_corrupt_kvs == replica_bucket_metadata_index_oid:
            key = r'123456789/' + KV['Key']
        else:
            key = KV['Key']        
        status, res = CORTXS3KVApi(config)\
            .put(index_with_correct_kvs, key, KV['Value'])
        assert status == True
        assert isinstance(res, CORTXS3SuccessResponse)

# ***************************************************************************************************
# Test: recover corrupted, old and missing key-values in primary index from replica index 
# Step 1: PUT 4 Key-Values (K1V1, K2V2, K3V3, K4V4) in primary index, including 1 corrupted key-value.
# Step 2: Put 4 Keys but different Values (K1V'1, K2V2, K3V'3, K5V5) in replica index.
# Step 3: Run s3 recoverytool with --recover option.
# Step 4: Validate that both primary and replica index have 5 key-values,
# with no key having corrupted value, both still having the same K2V2 and K3 has V'3 value in both indexes.
# Step 5: Delete the Key-Values from the primary and replica indexes.

# PUT KVs in the primary and replica indexes of global bucket list
# primary index would have corrupt and old KVs, replica index would have
# correct and latest KVS
put_kvs_in_indexes(primary_bucket_list_index_oid, replica_bucket_list_index_oid)

# PUT KVs in the primary and replica indexes of bucket metadata
# primary index would have corrupt and old KVs, replica index would have
# correct and latest KVS
put_kvs_in_indexes(primary_bucket_metadata_index_oid, replica_bucket_metadata_index_oid)

# Validate that primary indexes had 5 keys before running the s3recoverytool
status, res = CORTXS3IndexApi(config).list(primary_bucket_list_index_oid)
assert status == True
assert isinstance(res, CORTXS3ListIndexResponse)

index_content = res.get_index_content()
key_value_primary_index_before_recovery_list = index_content["Keys"]
assert index_content["Index-Id"] == primary_bucket_list_index_oid
assert len(key_value_primary_index_before_recovery_list) == 5

status, res = CORTXS3IndexApi(config).list(primary_bucket_metadata_index_oid)
assert status == True
assert isinstance(res, CORTXS3ListIndexResponse)

index_content = res.get_index_content()
key_value_primary_index_before_recovery_list = index_content["Keys"]
assert index_content["Index-Id"] == primary_bucket_metadata_index_oid
assert len(key_value_primary_index_before_recovery_list) == 5

# Validate that replica indexes had 4 keys before running the s3recoverytool
status, res = CORTXS3IndexApi(config).list(replica_bucket_list_index_oid)
assert status == True
assert isinstance(res, CORTXS3ListIndexResponse)

index_content = res.get_index_content()
key_value_replica_index_before_recovery_list = index_content["Keys"]
assert index_content["Index-Id"] == replica_bucket_list_index_oid
assert len(key_value_replica_index_before_recovery_list) == 4

status, res = CORTXS3IndexApi(config).list(replica_bucket_metadata_index_oid)
assert status == True
assert isinstance(res, CORTXS3ListIndexResponse)

index_content = res.get_index_content()
key_value_replica_index_before_recovery_list = index_content["Keys"]
assert index_content["Index-Id"] == replica_bucket_metadata_index_oid
assert len(key_value_replica_index_before_recovery_list) == 4

# Run s3 recovery tool
result = S3RecoveryTest("s3recovery --recover (corrupted and missing KVs in primary index)")\
    .s3recovery_recover()\
    .execute_test()\
    .command_is_successful()

# List Primary Indexes
status, res = CORTXS3IndexApi(config).list(primary_bucket_list_index_oid)
assert status == True
assert isinstance(res, CORTXS3ListIndexResponse)

index_content = res.get_index_content()
assert index_content["Index-Id"] == primary_bucket_list_index_oid
actual_primary_list_index_key_value_list = index_content["Keys"]
assert len(actual_primary_list_index_key_value_list) == 5

status, res = CORTXS3IndexApi(config).list(primary_bucket_metadata_index_oid)
assert status == True
assert isinstance(res, CORTXS3ListIndexResponse)

index_content = res.get_index_content()
assert index_content["Index-Id"] == primary_bucket_metadata_index_oid
actual_primary_metadata_index_key_value_list = index_content["Keys"]
assert len(actual_primary_metadata_index_key_value_list) == 5

# List replica Indexes
status, res = CORTXS3IndexApi(config).list(replica_bucket_list_index_oid)
assert status == True
assert isinstance(res, CORTXS3ListIndexResponse)

index_content = res.get_index_content()
assert index_content["Index-Id"] == replica_bucket_list_index_oid
actual_replica_list_index_key_value_list = index_content["Keys"]
assert len(actual_replica_list_index_key_value_list) == 5

status, res = CORTXS3IndexApi(config).list(replica_bucket_metadata_index_oid)
assert status == True
assert isinstance(res, CORTXS3ListIndexResponse)

index_content = res.get_index_content()
assert index_content["Index-Id"] == replica_bucket_metadata_index_oid
actual_replica_metadata_index_key_value_list = index_content["Keys"]
assert len(actual_replica_metadata_index_key_value_list) == 5

# validate if both primary and replica index have same keys and values
validate_if_same_kvs(actual_primary_list_index_key_value_list,
    actual_replica_list_index_key_value_list)

validate_if_same_kvs(actual_primary_metadata_index_key_value_list,
    actual_replica_metadata_index_key_value_list)

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

    status, res = CORTXS3KVApi(config)\
        .delete(replica_bucket_metadata_index_oid, metadata_index_key)
    assert status == True
    assert isinstance(res, CORTXS3SuccessResponse)

# ***************************************************************************************************
# Test: recover corrupted, old and missing key-values in replica index from primary index 
# Step 1: PUT 4 Key-Values (K1V1, K2V2, K3V3, K4V4) in replica index, including 1 corrupted key-value.
# Step 2: Put 4 Keys but different Values (K1V'1, K2V2, K3V'3, K5V5) in primary index.
# Step 3: Run s3 recoverytool with --recover option.
# Step 4: Validate that both primary and replica index have 5 key-values,
# with no key having corrupted value, both still having the same K2V2 and K3 has V'3 value in both indexes.
# Step 5: Delete the Key-Values from the primary and replica indexes.

# PUT KVs in the replica and primary indexes of global bucket list
# replica index would have corrupt and old KVs, primary index would have
# correct and latest KVS
put_kvs_in_indexes(replica_bucket_list_index_oid, primary_bucket_list_index_oid)

# PUT KVs in the replica and primary indexes of bucket metadata
# replica index would have corrupt and old KVs, primary index would have
# correct and latest KVS
put_kvs_in_indexes(replica_bucket_metadata_index_oid, primary_bucket_metadata_index_oid)

# Validate that primary indexes have 4 keys before running the s3recoverytool
status, res = CORTXS3IndexApi(config).list(primary_bucket_list_index_oid)
assert status == True
assert isinstance(res, CORTXS3ListIndexResponse)

index_content = res.get_index_content()
key_value_primary_index_before_recovery_list = index_content["Keys"]
assert index_content["Index-Id"] == primary_bucket_list_index_oid
assert len(key_value_primary_index_before_recovery_list) == 4

status, res = CORTXS3IndexApi(config).list(primary_bucket_metadata_index_oid)
assert status == True
assert isinstance(res, CORTXS3ListIndexResponse)

index_content = res.get_index_content()
key_value_primary_index_before_recovery_list = index_content["Keys"]
assert index_content["Index-Id"] == primary_bucket_metadata_index_oid
assert len(key_value_primary_index_before_recovery_list) == 4

# Validate that replica indexes have 4 keys before running the s3recoverytool
status, res = CORTXS3IndexApi(config).list(replica_bucket_list_index_oid)
assert status == True
assert isinstance(res, CORTXS3ListIndexResponse)

index_content = res.get_index_content()
key_value_replica_index_before_recovery_list = index_content["Keys"]
assert index_content["Index-Id"] == replica_bucket_list_index_oid
assert len(key_value_replica_index_before_recovery_list) == 5

status, res = CORTXS3IndexApi(config).list(replica_bucket_metadata_index_oid)
assert status == True
assert isinstance(res, CORTXS3ListIndexResponse)

index_content = res.get_index_content()
key_value_replica_index_before_recovery_list = index_content["Keys"]
assert index_content["Index-Id"] == replica_bucket_metadata_index_oid
assert len(key_value_replica_index_before_recovery_list) == 5

# Run s3 recovery tool
result = S3RecoveryTest("s3recovery --recover (corrupted and missing KVs in replica index)")\
    .s3recovery_recover()\
    .execute_test()\
    .command_is_successful()

# List Primary Indexes
status, res = CORTXS3IndexApi(config).list(primary_bucket_list_index_oid)
assert status == True
assert isinstance(res, CORTXS3ListIndexResponse)

index_content = res.get_index_content()
assert index_content["Index-Id"] == primary_bucket_list_index_oid
actual_primary_list_index_key_value_list = index_content["Keys"]
assert len(actual_primary_list_index_key_value_list) == 5

status, res = CORTXS3IndexApi(config).list(primary_bucket_metadata_index_oid)
assert status == True
assert isinstance(res, CORTXS3ListIndexResponse)

index_content = res.get_index_content()
assert index_content["Index-Id"] == primary_bucket_metadata_index_oid
actual_primary_metadata_index_key_value_list = index_content["Keys"]
assert len(actual_primary_metadata_index_key_value_list) == 5

# List replica Indexes
status, res = CORTXS3IndexApi(config).list(replica_bucket_list_index_oid)
assert status == True
assert isinstance(res, CORTXS3ListIndexResponse)

index_content = res.get_index_content()
assert index_content["Index-Id"] == replica_bucket_list_index_oid
actual_replica_list_index_key_value_list = index_content["Keys"]
assert len(actual_replica_list_index_key_value_list) == 5

status, res = CORTXS3IndexApi(config).list(replica_bucket_metadata_index_oid)
assert status == True
assert isinstance(res, CORTXS3ListIndexResponse)

index_content = res.get_index_content()
assert index_content["Index-Id"] == replica_bucket_metadata_index_oid
actual_replica_metadata_index_key_value_list = index_content["Keys"]
assert len(actual_replica_metadata_index_key_value_list) == 5

# validate if both primary and replica index have same keys and values
validate_if_same_kvs(actual_primary_list_index_key_value_list,
    actual_replica_list_index_key_value_list)

validate_if_same_kvs(actual_primary_metadata_index_key_value_list,
    actual_replica_metadata_index_key_value_list)

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

    status, res = CORTXS3KVApi(config)\
        .delete(replica_bucket_metadata_index_oid, metadata_index_key)
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
        'Key': r'123456789/my-bucket10',
        'Value': '{"ACL":"", "Bucket-Name":"my-bucket10", "Policy":"",\
             "System-Defined": { "Date":"2020-01-01T06:01:40.000Z", "LocationConstraint":"EU",\
             "Owner-Account":"temp-acc", "Owner-Account-id":"123456789", "Owner-User":"root",\
             "Owner-User-id":"HzG2EFe-RaWSyeecGAKafQ" }, "create_timestamp":"2020-01-01T06:01:40.000Z",\
             "motr_multipart_index_oid":"lwnlAAAAAHg=-FAAAAAAAlko=",\
             "motr_object_list_index_oid":"lwnlAAAAAHg=-EwAAAAAAlko=",\
             "motr_objects_version_list_index_oid":"lwnlAAAAAHg=-FQAAAAAAlko=" }'
    },
    {
        'Key': r'123456789/my-bucket11',
        'Value': '{"ACL":"", "Bucket-Name":"my-bucket11", "Policy":"",\
             "System-Defined": { "Date":"2020-01-01T06:05:40.000Z", "LocationConstraint":"sa-east-1",\
             "Owner-Account":"temp-acc", "Owner-Account-id":"123456789", "Owner-User":"root",\
             "Owner-User-id":"HzG2EFe-RaWSyeecGAKafQ" }, "create_timestamp":"2020-01-01T06:05:40.000Z",\
             "motr_multipart_index_oid":"lwnlAAAAAHg=-FAAAAAAAlko=",\
             "motr_object_list_index_oid":"lwnlAAAAAHg=-EwAAAAAAlko=",\
             "motr_objects_version_list_index_oid":"lwnlAAAAAHg=-FQAAAAAAlko=" }'
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
    status, res = CORTXS3KVApi(config)\
        .put(primary_bucket_metadata_index_oid, KV['Key'], KV['Value'])
    assert status == True
    assert isinstance(res, CORTXS3SuccessResponse)

    status, res = CORTXS3KVApi(config)\
        .put(replica_bucket_metadata_index_oid, KV['Key'], KV['Value'])
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
key_value_primary_list_index_after_recovery_list = index_content["Keys"]
assert index_content["Index-Id"] == primary_bucket_list_index_oid
assert len(key_value_primary_list_index_after_recovery_list) == 2

status, res = CORTXS3IndexApi(config).list(replica_bucket_list_index_oid)
assert status == True
assert isinstance(res, CORTXS3ListIndexResponse)

index_content = res.get_index_content()
key_value_replica_list_index_after_recovery_list = index_content["Keys"]
assert index_content["Index-Id"] == replica_bucket_list_index_oid
assert len(key_value_replica_list_index_after_recovery_list) == 2

# Validate 2 KVs in global bucket metadata index after doing 's3recovery --recover' 
status, res = CORTXS3IndexApi(config).list(primary_bucket_metadata_index_oid)
assert status == True
assert isinstance(res, CORTXS3ListIndexResponse)

index_content = res.get_index_content()
key_value_primary_metadata_index_after_recovery_list = index_content["Keys"]
assert index_content["Index-Id"] == primary_bucket_metadata_index_oid
assert len(key_value_primary_metadata_index_after_recovery_list) == 2
 
# Delete the key-values from both primary and replica indexes
for KV in global_bucket_list_index_key_value_list:
    status, res = CORTXS3KVApi(config).delete(primary_bucket_list_index_oid, KV['Key'])
    assert status == True
    assert isinstance(res, CORTXS3SuccessResponse)

    status, res = CORTXS3KVApi(config).delete(replica_bucket_list_index_oid, KV['Key'])
    assert status == True
    assert isinstance(res, CORTXS3SuccessResponse)

for KV in global_bucket_metadata_index_key_value_list:
    status, res = CORTXS3KVApi(config)\
        .delete(primary_bucket_metadata_index_oid, KV['Key'])
    assert status == True
    assert isinstance(res, CORTXS3SuccessResponse)

    status, res = CORTXS3KVApi(config)\
        .delete(replica_bucket_metadata_index_oid, KV['Key'])
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
