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
            config['cortx_s3']['access_key'] = access_key_value
            config['cortx_s3']['secret_key'] = secret_key_value
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

replica_bucket_list_index_oid = 'AAAAAAAAAHg=-BQAQAAAAAAA=' # base64 conversion of 0x7800000000000000" and "0x100005
primary_bucket_list_index_oid = 'AAAAAAAAAHg=-AQAQAAAAAAA='
config = CORTXS3Config()

# ======================================================================================================

# Test Scenario 1
# Test if Replica of global bucket list index OID present.
print("\nvalidate if replica global bucket index exists.\n")
status, res = CORTXS3IndexApi(config).head(replica_bucket_list_index_oid)
assert status == True
assert isinstance(res, CORTXS3SuccessResponse)
print("\nHEAD 'replica index' validation completed.\n")

# ******************************************************************************************************

# Test Scenario 2
# Test if KV is created in replica global bucket list index at PUT bucket
print("\nvalidate if KV created in replica global bucket list index at PUT bucket\n")
AwsTest('Create Bucket "seagatebucket" using s3-recovery-svc account')\
    .create_bucket("seagatebucket").execute_test().command_is_successful()

status, res = CORTXS3IndexApi(config).list(replica_bucket_list_index_oid)
assert status == True
assert isinstance(res, CORTXS3ListIndexResponse)
# Example index_content:
# {'Delimiter': '',
#  'Index-Id': 'AAAAAAAAAHg=-BQAQAAAAAAA=',
#  'IsTruncated': 'false',
#  'Keys': [
#       {'Key': 'seagatebucket',
#        'Value': '{"account_id":"293986807303","account_name":"s3-recovery-svc","create_timestamp":"2020-06-16T04:46:46.000Z","location_constraint":"us-west-2"}\n'}
#   ],
#   'Marker': '',
#   'MaxKeys': '1000',
#   'NextMarker': '', 'Prefix': ''
# }
index_content = res.get_index_content()
assert index_content["Index-Id"] == "AAAAAAAAAHg=-BQAQAAAAAAA="

bucket_key_value_list = index_content["Keys"]
bucket_key_value = (bucket_key_value_list[0])
bucket_key = bucket_key_value["Key"]
bucket_value = bucket_key_value["Value"]

value_json = json.loads(bucket_value)

assert bucket_key == "seagatebucket"
assert value_json['account_id'] == account_response_elements["AccountId"]
assert value_json['create_timestamp'] != ''

# ******************************************************************************************************

# Test Scenario 3
# Test if KV is removed from replica global bucket list index at DELETE bucket
print("\nvalidate if KV deleted from replica global bucket list index at DELETE bucket\n")
AwsTest('Delete Bucket "seagatebucket"').delete_bucket("seagatebucket")\
   .execute_test().command_is_successful()

status, res = CORTXS3IndexApi(config).list(replica_bucket_list_index_oid)

assert status == True
assert isinstance(res, CORTXS3ListIndexResponse)

index_content = res.get_index_content()
assert index_content["Index-Id"] == "AAAAAAAAAHg=-BQAQAAAAAAA="
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

root_bucket_account_list_index = S3kvTest('KvTest fetch root bucket account index')\
    .root_bucket_account_index()

primary_index_key = "my-bucket3"
primary_index_corrupted_value = "Corrupted-JSON-value"
S3kvTest('KvTest put corrupted key-value in root bucket account index')\
    .put_keyval(root_bucket_account_list_index, primary_index_key, primary_index_corrupted_value)\
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

# ================================================= CLEANUP =============================================

# ************ Delete Account*******************************
test_msg = "Delete account s3-recovery-svc"
account_args = {'AccountName': 's3-recovery-svc',\
               'Email': 's3-recovery-svc@seagate.com', 'force': True}
AuthTest(test_msg).delete_account(**account_args).execute_test()\
    .command_response_should_have("Account deleted successfully")

# Restore s3backgrounddelete config file.
restore_configuration()
