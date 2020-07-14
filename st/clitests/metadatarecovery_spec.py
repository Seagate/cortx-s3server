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
import s3kvs

from s3backgrounddelete.eos_core_config import EOSCoreConfig
from s3backgrounddelete.eos_core_kv_api import EOSCoreKVApi
from s3backgrounddelete.eos_core_index_api import EOSCoreIndexApi
from s3backgrounddelete.eos_core_error_respose import EOSCoreErrorResponse
from s3backgrounddelete.eos_list_index_response import EOSCoreListIndexResponse
from s3backgrounddelete.eos_core_success_response import EOSCoreSuccessResponse

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
# We are using s3backgrounddelete config file as EOSCoreConfig is tightly coupled with it.
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
            config['eos_core']['access_key'] = access_key_value
            config['eos_core']['secret_key'] = secret_key_value
            config['eos_core']['daemon_mode'] = "False"
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
config = EOSCoreConfig()

# ======================================================================================================

# Test Scenario 1
# Test if Replica of global bucket list index OID present.
print("\nvalidate if replica global bucket index exists.\n")
status, res = EOSCoreIndexApi(config).head(replica_bucket_list_index_oid)
assert status == True
assert isinstance(res, EOSCoreSuccessResponse)
print("\nHEAD 'replica index' validation completed.\n")

# ******************************************************************************************************

# Test Scenario 2
# Test if KV is created in replica global bucket list index at PUT bucket
print("\nvalidate if KV created in replica global bucket list index at PUT bucket\n")
AwsTest('Create Bucket "seagatebucket" using s3-recovery-svc account')\
    .create_bucket("seagatebucket").execute_test().command_is_successful()

status, res = EOSCoreIndexApi(config).list(replica_bucket_list_index_oid)
assert status == True
assert isinstance(res, EOSCoreListIndexResponse)
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

status, res = EOSCoreIndexApi(config).list(replica_bucket_list_index_oid)

assert status == True
assert isinstance(res, EOSCoreListIndexResponse)

index_content = res.get_index_content()
assert index_content["Index-Id"] == "AAAAAAAAAHg=-BQAQAAAAAAA="
assert index_content["Keys"] == None

# ********************** System Tests for s3 recovery tool: dry_run option ********************************************
# Run s3 recovery tool: dry_run option
# Test: KV missing from primary index, but present in replica index
# Step 1: PUT Key-Value in replica index
# Step 2: Run s3 recoverytool with --dry_run option
# Step 3: Validate that the Key-Value is shown on the console as data recovered
# Step 4: Delete the Key-Value from the replica index

print("Test: validate dry_run: KV missing from primary index, but present in replica index\n")

st1key = "ST-1-BK"
st1value = '{"account_id":"838334245437",\
     "account_name":"s3-recovery-svc",\
     "create_timestamp":"2020-07-02T05:45:41.000Z",\
     "location_constraint":"us-west-2"}'

# ***************** PUT KV in replica index **********************************
status, res = EOSCoreKVApi(config).put(replica_bucket_list_index_oid, st1key, st1value)
assert status == True
assert isinstance(res, EOSCoreSuccessResponse)

# Make sure that root index is empty currently
status, res = EOSCoreIndexApi(config).list(primary_bucket_list_index_oid)
assert status == True
assert isinstance(res, EOSCoreListIndexResponse)

index_content = res.get_index_content()
assert index_content["Index-Id"] == "AAAAAAAAAHg=-AQAQAAAAAAA="
assert index_content["Keys"] == None

# Run s3 recovery tool
result = S3RecoveryTest("run s3recovery tool")\
    .s3recovery_dry_run()\
    .execute_test()\
    .command_is_successful()
print(result.status.stdout)
success_msg = "Data recovered from both indexes for Global bucket index"
result.command_response_should_have(success_msg)

# Delete the key-value from replica index
status, res = EOSCoreKVApi(config).delete(replica_bucket_list_index_oid, st1key)
assert status == True
assert isinstance(res, EOSCoreSuccessResponse)

# ================================================= CLEANUP =============================================

# ************ Delete Account*******************************
test_msg = "Delete account s3-recovery-svc"
account_args = {'AccountName': 's3-recovery-svc',\
               'Email': 's3-recovery-svc@seagate.com', 'force': True}
AuthTest(test_msg).delete_account(**account_args).execute_test()\
    .command_response_should_have("Account deleted successfully")

# Restore s3backgrounddelete config file.
restore_configuration()
