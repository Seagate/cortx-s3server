import os
import sys
import time
import subprocess
from framework import S3PyCliTest
from framework import Config
from framework import logit

class ClovisConfig():
    def __init__(self):
        lctl_cmd = "sudo lctl list_nids | head -1"
        result = subprocess.check_output(lctl_cmd, shell=True).decode().split()[0]
        self.LOCAL_NID = result
        self.LOCAL_EP = self.LOCAL_NID + ":12345:33:101"
        self.HA_EP = self.LOCAL_NID + ":12345:34:1"
        self.CONFD_EP = self.LOCAL_NID + ":12345:44:101"

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
        clovis_conf = ClovisConfig()
        self.cmd = "sudo cloviskvscli"
        self.common_args = " --clovis_local_addr=" + clovis_conf.LOCAL_EP  + " --clovis_ha_addr=" + clovis_conf.HA_EP + " --clovis_confd_addr=" + clovis_conf.CONFD_EP + " "
        super(S3kvTest, self).__init__(description)

    def root_index_records(self):
        root_oid = self.root_index()
        kvs_cmd = self.cmd + self.common_args + " --index_hi=" + root_oid.oid_hi + " --index_lo=" + root_oid.oid_lo + " --op_count=3 --action=next"
        self.with_cli(kvs_cmd)
        return self

# Root index table OID used by S3server is a constant derived by
# adding 1 to  M0_CLOVIS_ID_APP define and packing it using M0_FID_TINIT
    def root_index(self):
        root_oid = S3OID("0x6900000000000000", "0x100001")
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
