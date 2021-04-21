#
# Copyright (c) 2021 Seagate Technology LLC and/or its Affiliates
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

import shutil
from framework import Config
from ldap_setup import LdapInfo
from framework import S3PyCliTest
from s3client_config import S3ClientConfig
from s3fi import S3fiTest
from awss3api import AwsTest
import s3kvs
import hashlib

def file_md5(fname):
    hash_md5 = hashlib.md5()
    with open(fname, "rb") as f:
        for chunk in iter(lambda: f.read(4096), b""):
            hash_md5.update(chunk)
    return hash_md5.hexdigest()

# Run before all to setup the test environment.
def before_all():
    print("Configuring LDAP")
    S3PyCliTest('Before_all').before_all()

# Run before all to setup the test environment.
before_all()

S3ClientConfig.pathstyle = False
S3ClientConfig.ldapuser = 'sgiamadmin'
S3ClientConfig.ldappasswd = LdapInfo.get_ldap_admin_pwd()
Config.no_ssl = True
Config.log_enabled = False

# Set access key, secret key of s3_test account
S3ClientConfig.access_key_id = 'AKIAJPINPFRBTPAYOGNA'
S3ClientConfig.secret_key = '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'

# Clear all S3 index data
s3kvs.clean_all_data()
s3kvs.create_s3root_index()

# ********** Create 's3faultbucket' bucket in existing s3_test account********
AwsTest('Create Bucket "s3faultbucket" using s3_test account')\
    .create_bucket("s3faultbucket").execute_test().command_is_successful()

# Enable S3 Fault tolerence mode by setting S3_MAX_EXTENDED_OBJECTS_IN_FAULT_MODE to non-zero value
# TODO:
# 1. Change /opt/seagate/cortx/s3/conf/s3config.yaml
# 2. Re-start S3 server or force S3 server to reload new config changes

# Enable S3 FI to force object write failure to allow creation of fragments
S3fiTest('s3cmd enable FI motr_obj_write_fail').enable_fi_offnonm("enable", "motr_obj_write_fail", "1", "1").execute_test().command_is_successful()

# Upload object (of size 4.7MB) to bucket.
# It will create 4 additional objects/fragments (each of size 1MB, last being 700KB)
# along with the main object (of size 1MB)
aws_test = AwsTest('Aws can create fragmented object').put_object("s3faultbucket", "4_7_mbfile", 4700000, key_name="4_7MB")\
    .execute_test(need_MD5=True).command_is_successful()

# Store MD5 hash of uploaded object/file
upload_MD5_hash = aws_test.fileMD5

# Disable S3 FI
S3fiTest('s3cmd disable Fault injection').disable_fi("motr_obj_write_fail").execute_test().command_is_successful()

# ********** Validate uploaded object using S3 metadata information **********
# Validate FNo=4 and size of each fragment (except the last fragment) is 1MB
obj_md, frag_info = s3kvs.fetch_object_info("s3faultbucket", "4_7MB")
assert obj_md is None or not hasattr(obj_md, 'FNo'), "Error! Object metadata or FNo in metadata is missing"
assert obj_md["FNo"] == 4, "Error! Fragment count (FNo) should be 4"
assert frag_info is not None and len(frag_info) == 4, "Error! No fragments found or count does not match 4"

# Validate object by downloading it and comparing it's MD5 with the uploaded object
aws_test = AwsTest('Aws can download fragmented object').get_object("s3faultbucket", "4_7MB")\
    .execute_test(need_MD5=True).command_is_successful()

# Store MD5 hash of downloaded object/file
download_MD5_hash = aws_test.fileMD5
assert str(upload_MD5_hash) == str(download_MD5_hash), "Error! File hash of uploaded object not matching that of downloaded object"

# Delete fragmented object.
# Ensure that all entries associated with fragments are removed from object list index
AwsTest('Aws can delete fragmented object').delete_object("s3faultbucket", "4_7MB")\
    .execute_test().command_is_successful()
# Ensure no entries (including fragments) in object list index
obj_md, frag_info = s3kvs.fetch_object_info("s3faultbucket", "4_7MB", deep_frag_check=True)
assert obj_md is None or (frag_info is None or len(frag_info) !=0), "Error! Fragments still exist in object list index"

