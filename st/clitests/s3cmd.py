
import os
import sys
import time
from threading import Timer
import subprocess
from framework import S3PyCliTest
from framework import Config
from framework import logit

class S3cmdTest(S3PyCliTest):
    def __init__(self, description):
        self.s3cfg = os.path.join(os.path.dirname(os.path.realpath(__file__)), Config.config_file)
        super(S3cmdTest, self).__init__(description)

    def setup(self):
        if hasattr(self, 'filename') and hasattr(self, 'filesize'):
            file_to_create = os.path.join(self.working_dir, self.filename)
            logit("Creating file [%s] with size [%d]" % (file_to_create, self.filesize))
            with open(file_to_create, 'wb') as fout:
                fout.write(os.urandom(self.filesize))
        super(S3cmdTest, self).setup()

    def run(self):
        super(S3cmdTest, self).run()

    def teardown(self):
        super(S3cmdTest, self).teardown()

    def create_bucket(self, bucket_name, region=None):
        self.bucket_name = bucket_name
        if region:
            self.with_cli("s3cmd -c " + self.s3cfg + " mb " + " s3://" + self.bucket_name + " --bucket-location=" + region)
        else:
            self.with_cli("s3cmd -c " + self.s3cfg + " mb " + " s3://" + self.bucket_name)
        return self

    def list_buckets(self):
        self.with_cli("s3cmd -c " + self.s3cfg + " ls ")
        return self

    def info_bucket(self, bucket_name):
        self.bucket_name = bucket_name
        self.with_cli("s3cmd -c " + self.s3cfg + " info " + " s3://" + self.bucket_name)
        return self

    def info_object(self, bucket_name, filename):
        self.bucket_name = bucket_name
        self.filename = filename
        self.with_cli("s3cmd -c " + self.s3cfg + " info " + " s3://" + self.bucket_name + "/" + self.filename)
        return self

    def delete_bucket(self, bucket_name):
        self.bucket_name = bucket_name
        self.with_cli("s3cmd -c " + self.s3cfg + " rb " + " s3://" + self.bucket_name)
        return self

    def list_objects(self, bucket_name):
        self.bucket_name = bucket_name
        self.with_cli("s3cmd -c " + self.s3cfg + " ls " + " s3://" + self.bucket_name)
        return self

    def list_all_objects(self):
        self.with_cli("s3cmd -c " + self.s3cfg + " la ")
        return self

    def list_specific_objects(self, bucket_name, object_pattern):
        self.bucket_name = bucket_name
        self.object_pattern = object_pattern
        self.with_cli("s3cmd -c " + self.s3cfg + " ls " + " s3://" + self.bucket_name + "/" + self.object_pattern)
        return self

    def disk_usage_bucket(self, bucket_name):
        self.bucket_name = bucket_name
        self.with_cli("s3cmd -c " + self.s3cfg + " du " + " s3://" + self.bucket_name)
        return self

    def upload_test(self, bucket_name, filename, filesize):
        self.filename = filename
        self.filesize = filesize
        self.bucket_name = bucket_name
        self.with_cli("s3cmd -c " + self.s3cfg + " put " + self.filename + " s3://" + self.bucket_name)
        return self

    def upload_copy_test(self, bucket_name, srcfile, destfile):
        self.srcfile = srcfile
        self.destfile = destfile
        self.bucket_name = bucket_name
        self.with_cli("s3cmd -c " + self.s3cfg + " cp s3://" + self.bucket_name + "/" + self.srcfile + " s3://" + self.bucket_name + "/" + self.destfile)
        return self

    def upload_move_test(self, src_bucket, srcfile, dest_bucket, destfile):
        self.srcfile = srcfile
        self.destfile = destfile
        self.src_bucket = src_bucket
        self.dest_bucket = dest_bucket
        self.with_cli("s3cmd -c " + self.s3cfg + " cp s3://" + self.src_bucket + "/" + self.srcfile + " s3://" + self.dest_bucket + "/" + self.destfile)
        return self

    def list_multipart_uploads(self, bucket_name):
        self.bucket_name = bucket_name
        self.with_cli("s3cmd -c " + self.s3cfg + " multipart s3://" + self.bucket_name)
        return self

    def abort_multipart(self, bucket_name, filename, upload_id):
        self.filename = filename
        self.bucket_name = bucket_name
        self.with_cli("s3cmd -c " + self.s3cfg + " abortmp s3://" + self.bucket_name + "/" + self.filename + " " + upload_id)
        return self

    def list_parts(self, bucket_name, filename, upload_id):
        self.filename = filename
        self.bucket_name = bucket_name
        cmd = "s3cmd -c %s listmp s3://%s/%s %s" % (self.s3cfg, bucket_name,
            filename, upload_id)

        self.with_cli(cmd)
        return self

    def download_test(self, bucket_name, filename):
        self.filename = filename
        self.bucket_name = bucket_name
        self.with_cli("s3cmd -c " + self.s3cfg + " get " + "s3://" + self.bucket_name + "/" + self.filename)
        return self

    def setacl_bucket(self, bucket_name, acl_perm):
        self.bucket_name = bucket_name
        self.with_cli("s3cmd -c " + self.s3cfg + " setacl " + "s3://" + self.bucket_name + " --acl-grant=" + acl_perm)
        return self

    def setpolicy_bucket(self, bucket_name, policyfile):
        self.bucket_name = bucket_name
        self.with_cli("s3cmd -c " + self.s3cfg + " setpolicy " + self.working_dir + "/../" + policyfile + " s3://" + self.bucket_name)
        return self

    def delpolicy_bucket(self, bucket_name):
        self.bucket_name = bucket_name
        self.with_cli("s3cmd -c " + self.s3cfg + " delpolicy " + " s3://" + self.bucket_name)
        return self

    def accesslog_bucket(self, bucket_name):
        self.bucket_name = bucket_name
        self.with_cli("s3cmd -c " + self.s3cfg + " accesslog " + " s3://" + self.bucket_name)
        return self

    def fixbucket(self, bucket_name):
        self.bucket_name = bucket_name
        self.with_cli("s3cmd -c " + self.s3cfg + " fixbucket " + " s3://" + self.bucket_name)
        return self

    def setacl_object(self, bucket_name, filename, acl_perm):
        self.filename = filename
        self.bucket_name = bucket_name
        self.with_cli("s3cmd -c " + self.s3cfg + " setacl " + "s3://" + self.bucket_name + "/" + self.filename + " --acl-grant=" + acl_perm)
        return self

    def revoke_acl_bucket(self, bucket_name, acl_perm):
        self.bucket_name = bucket_name
        self.with_cli("s3cmd -c " + self.s3cfg + " setacl " + "s3://" + self.bucket_name + " --acl-revoke=" + acl_perm)
        return self

    def revoke_acl_object(self, bucket_name, filename, acl_perm):
        self.filename = filename
        self.bucket_name = bucket_name
        self.with_cli("s3cmd -c " + self.s3cfg + " setacl " + "s3://" + self.bucket_name + "/" + self.filename + " --acl-revoke=" + acl_perm)
        return self

    def stop_s3authserver_test(self):
        cmd = "sudo systemctl stop s3authserver.service";
        self.with_cli(cmd)
        return self

    def start_s3authserver_test(self):
        cmd = "sudo systemctl start s3authserver.service";
        self.with_cli(cmd)
        return self

    def delete_test(self, bucket_name, filename):
        self.filename = filename
        self.bucket_name = bucket_name
        self.with_cli("s3cmd -c " + self.s3cfg + " del " + "s3://" + self.bucket_name + "/" + self.filename)
        return self

    def multi_delete_test(self, bucket_name):
        self.bucket_name = bucket_name
        self.with_cli("s3cmd -c " + self.s3cfg + " del " + "s3://" + self.bucket_name + "/ --recursive --force")
        return self
