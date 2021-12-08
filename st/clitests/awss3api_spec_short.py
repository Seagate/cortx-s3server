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

import os
import yaml
import hashlib
from framework import S3PyCliTest
from awss3api import AwsTest
from s3client_config import S3ClientConfig
from auth import AuthTest
# Helps debugging
# Config.log_enabled = True
# Config.dummy_run = True
# Config.client_execution_timeout = 300 * 1000
# Config.request_timeout = 300 * 1000
# Config.socket_timeout = 300 * 1000

# Transform AWS CLI text output into an object(dictionary)
# with content:
# {
#   "prefix":<list of common prefix>,
#   "keys": <list of regular keys>,
#   "next_token": <token>
# }
def get_aws_cli_object(raw_aws_cli_output):
    cli_obj = {}
    raw_lines = raw_aws_cli_output.split('\n')
    common_prefixes = []
    content_keys = []
    for _, item in enumerate(raw_lines):
        if (item.startswith("COMMONPREFIXES")):
            # E.g. COMMONPREFIXES  quax/
            line = item.split('\t')
            common_prefixes.append(line[1])
        elif (item.startswith("CONTENTS")):
            # E.g. CONTENTS\t"98b5e3f766f63787ea1ddc35319cedf7"\tasdf\t2020-09-25T11:42:54.000Z\t3072\tSTANDARD
            line = item.split('\t')
            content_keys.append(line[2])
        elif (item.startswith("NEXTTOKEN")):
            # E.g. NEXTTOKEN       eyJDb250aW51YXRpb25Ub2tlbiI6IG51bGwsICJib3RvX3RydW5jYXRlX2Ftb3VudCI6IDN9
            line = item.split('\t')
            cli_obj["next_token"] = line[1]
        else:
            continue

    if (common_prefixes is not None):
        cli_obj["prefix"] = common_prefixes
    if (content_keys is not None):
        cli_obj["keys"] = content_keys

    return cli_obj

# Extract the upload id from response which has the following format
# [bucketname    objecctname    uploadid]

def get_upload_id(response):
    key_pairs = response.split('\t')
    return key_pairs[2]

def get_etag(response):
    key_pairs = response.split('\t')
    return key_pairs[3]

def load_test_config():
    conf_file = os.path.join(os.path.dirname(__file__),'s3iamcli_test_config.yaml')
    with open(conf_file, 'r') as f:
            config = yaml.safe_load(f)
            S3ClientConfig.ldapuser = config['ldapuser']
            S3ClientConfig.ldappasswd = config['ldappasswd']

def get_response_elements(response):
    response_elements = {}
    key_pairs = response.split(',')

    for key_pair in key_pairs:
        tokens = key_pair.split('=')
        response_elements[tokens[0].strip()] = tokens[1].strip()

    return response_elements

def create_object_list_file(file_name, obj_list=[], quiet_mode="false"):
    cwd = os.getcwd()
    file_to_create = os.path.join(cwd, file_name)
    objects = "{ \"Objects\": [ { \"Key\": \"" + obj_list[0] + "\" }"
    for obj in obj_list[1:]:
        objects += ", { \"Key\": \"" + obj + "\" }"
    objects += " ], \"Quiet\": " + quiet_mode + " }"
    with open(file_to_create, 'w') as file:
        file.write(objects)
    return file_to_create

def delete_object_list_file(file_name):
    os.remove(file_name)

def get_md5_sum(fname):
    hash_md5 = hashlib.md5()
    with open(fname, "rb") as f:
        for chunk in iter(lambda: f.read(4096), b""):
            hash_md5.update(chunk)
    return hash_md5.hexdigest()

# Helps debugging
# Config.log_enabled = True
# Config.dummy_run = True
# Config.client_execution_timeout = 300 * 1000
# Config.request_timeout = 300 * 1000
# Config.socket_timeout = 300 * 1000

# Run before all to setup the test environment.
print("Configuring LDAP")
S3PyCliTest('Before_all').before_all()
load_test_config()

#Create account sourceAccount
test_msg = "Create account sourceAccount"
source_account_args = {'AccountName': 'sourceAccount', 'Email': 'sourceAccount@seagate.com', \
                   'ldapuser': S3ClientConfig.ldapuser, \
                   'ldappasswd': S3ClientConfig.ldappasswd}
account_response_pattern = "AccountId = [\w-]*, CanonicalId = [\w-]*, RootUserName = [\w+=,.@-]*, AccessKeyId = [\w-]*, SecretKey = [\w/+]*$"
result1 = AuthTest(test_msg).create_account(**source_account_args).execute_test()
result1.command_should_match_pattern(account_response_pattern)
print(result1.status.stdout)
account_response_elements = get_response_elements(result1.status.stdout)
source_access_key_args = {}
source_access_key_args['AccountName'] = "sourceAccount"
source_access_key_args['AccessKeyId'] = account_response_elements['AccessKeyId']
source_access_key_args['SecretAccessKey'] = account_response_elements['SecretKey']

bucket_name = "seagatebuckettest"

#******** Create Bucket ********
AwsTest('Aws can create bucket').create_bucket(bucket_name)\
    .execute_test().command_is_successful()
###########################################
##### Add your STs here to test out #######
###########################################




#******** Delete Bucket ********
AwsTest('Aws can delete bucket').delete_bucket(bucket_name)\
    .execute_test().command_is_successful()

#-------------------Delete Accounts------------------
test_msg = "Delete account sourceAccount"
s3test_access_key = S3ClientConfig.access_key_id
s3test_secret_key = S3ClientConfig.secret_key
S3ClientConfig.access_key_id = source_access_key_args['AccessKeyId']
S3ClientConfig.secret_key = source_access_key_args['SecretAccessKey']

account_args = {'AccountName': 'sourceAccount', 'Email': 'sourceAccount@seagate.com',  'force': True}
AuthTest(test_msg).delete_account(**source_account_args).execute_test()\
            .command_response_should_have("Account deleted successfully")

