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
import sys
import time
import subprocess
import yaml
from framework import S3PyCliTest
from framework import Config
from framework import logit

class MotrConfig():
    def __init__(self):
        lctl_cmd = "sudo lctl list_nids | head -1"
        result = subprocess.check_output(lctl_cmd, shell=True).decode().split()[0]
        self.LOCAL_NID = result

        self.cfg_dir = os.path.join(os.path.dirname(__file__), 'cfg')
        config_file =  os.path.join(self.cfg_dir, 'motrkvscli.yaml')
        with open(config_file, 'r') as f:
            s3config = yaml.safe_load(f)
            self.KVS_IDX = str(s3config['S3_MOTR_IDX_SERVICE_ID'])
            self.LOCAL_EP = s3config['S3_MOTR_LOCAL_EP']
            self.HA_EP = s3config['S3_MOTR_HA_EP']
            self.PROFILE_FID = s3config['S3_MOTR_PROF']
            self.PROCESS_FID = s3config['S3_MOTR_PROCESS_FID']

        self.LOCAL_EP = self.LOCAL_NID + self.LOCAL_EP
        self.HA_EP = self.LOCAL_NID + self.HA_EP

class S3OID():
    def __init__(self, oid_hi="0x0", oid_lo="0x0"):
        self.oid_hi = oid_hi
        self.oid_lo = oid_lo

    def set_oid(self, oid_hi, oid_lo):
        self.oid_hi = oid_hi
        self.oid_lo = oid_lo
        return self

class S3kvTest(S3PyCliTest):
    def __init__(self, description):
        motr_conf = MotrConfig()
        if "LD_LIBRARY_PATH" in os.environ:
            self.cmd = "sudo env LD_LIBRARY_PATH=%s ../motrkvscli.sh" % os.environ["LD_LIBRARY_PATH"]
        else:
            self.cmd = "sudo ../motrkvscli.sh"
        self.common_args = " --motr_local_addr=" + motr_conf.LOCAL_EP  + " --motr_ha_addr=" + motr_conf.HA_EP + " --motr_profile=" + motr_conf.PROFILE_FID + " --motr_proc=" + motr_conf.PROCESS_FID + " --kvstore=" + motr_conf.KVS_IDX + " "
        super(S3kvTest, self).__init__(description)

    def root_bucket_account_index_records(self):
        root_oid = self.root_bucket_account_index()
        kvs_cmd = self.cmd + self.common_args + " --index_hi=" + root_oid.oid_hi + " --index_lo=" + root_oid.oid_lo + " --op_count=3 --action=next"
        self.with_cli(kvs_cmd)
        return self

    def root_bucket_metadata_index_records(self):
        root_oid = self.root_bucket_account_index()
        kvs_cmd = self.cmd + self.common_args + " --index_hi=" + root_oid.oid_hi + " --index_lo=" + root_oid.oid_lo + " --op_count=3 --action=next"
        self.with_cli(kvs_cmd)
        return self

    def root_probable_dead_object_list_index_records(self):
        root_oid = self.root_probable_dead_object_list_index()
        kvs_cmd = self.cmd + self.common_args + " --index_hi=" + root_oid.oid_hi + " --index_lo=" + root_oid.oid_lo + " --op_count=6 --action=next"
        self.with_cli(kvs_cmd)
        return self

# Root index table OID used by S3server is a constant derived by
# adding 1 to  M0_MOTR_ID_APP define and packing it using M0_FID_TINIT
    def root_bucket_account_index(self):
        root_oid = S3OID("0x7800000000000000", "0x100001")
        return root_oid

# Replica index table OID used by S3server is a constant derived by
# adding 5 to  M0_MOTR_ID_APP define and packing it using M0_FID_TINIT
    def replica_bucket_account_index(self):
        replica_oid = S3OID("0x7800000000000000", "0x100005")
        return replica_oid

# Root bucket metadata index table OID used by S3server is a constant derived by
# adding 2 to  M0_MOTR_ID_APP define and packing it using M0_FID_TINIT
    def root_bucket_metadata_index(self):
        root_oid = S3OID("0x7800000000000000", "0x100002")
        return root_oid

# Replica bucket metadata index table OID used by S3server is a constant derived by
# adding 6 to  M0_MOTR_ID_APP define and packing it using M0_FID_TINIT
    def replica_bucket_metadata_index(self):
        replica_oid = S3OID("0x7800000000000000", "0x100006")
        return replica_oid

# Root probable dead object list index table OID used by S3server is a constant derived by
# adding 3 to  M0_MOTR_ID_APP define and packing it using M0_FID_TINIT
    def root_probable_dead_object_list_index(self):
        root_oid = S3OID("0x7800000000000000", "0x100003")
        return root_oid

    def next_keyval(self, oid, key, count):
        kvs_cmd = self.cmd + self.common_args + " --index_hi=" + oid.oid_hi + " --index_lo=" + oid.oid_lo + " --action=next" + " --key=" + key +" --op_count=" + str(count)
        self.with_cli(kvs_cmd)
        return self

    def delete_keyval(self, oid, key):
        kvs_cmd = self.cmd + self.common_args + " --index_hi=" + oid.oid_hi + " --index_lo=" + oid.oid_lo + " --key=" + key + " --action=del"
        self.with_cli(kvs_cmd)
        return self

    def get_keyval(self, oid, key):
        kvs_cmd = self.cmd + self.common_args + " --index_hi=" + oid.oid_hi + " --index_lo=" + oid.oid_lo + " --key=" + key + " --action=get"
        self.with_cli(kvs_cmd)
        return self

    def put_keyval(self, oid, key, val):
        kvs_cmd = self.cmd + self.common_args + " --index_hi=" + oid.oid_hi + " --index_lo=" + oid.oid_lo + " --key=" + key + " --value=" + val + " --action=put"
        self.with_cli(kvs_cmd)
        return self

    def create_index(self, oid):
        kvs_cmd = self.cmd + self.common_args + " --index_hi=" + oid.oid_hi + " --index_lo=" + oid.oid_lo + " --action=createidx"
        self.with_cli(kvs_cmd)
        return self

    def delete_index(self, oid):
        kvs_cmd = self.cmd + self.common_args + " --index_hi=" + oid.oid_hi + " --index_lo=" + oid.oid_lo + " --action=deleteidx"
        self.with_cli(kvs_cmd)
        return self

    def create_root_bucket_account_index(self):
        root_oid = self.root_bucket_account_index()
        return self.create_index(root_oid)

    def delete_root_bucket_account_index(self):
        root_oid = self.root_bucket_account_index()
        return self.delete_index(root_oid)

    def create_root_bucket_metadata_index(self):
        root_oid = self.root_bucket_metadata_index()
        return self.create_index(root_oid)

    def delete_root_bucket_metadata_index(self):
        root_oid = self.root_bucket_metadata_index()
        return self.delete_index(root_oid)

    def create_root_probable_dead_object_list_index(self):
        root_oid = self.root_probable_dead_object_list_index()
        return self.create_index(root_oid)

    def delete_root_probable_dead_object_list_index(self):
        root_oid = self.root_probable_dead_object_list_index()
        return self.delete_index(root_oid)

