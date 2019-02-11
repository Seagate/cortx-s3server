import os
import sys
import time
import json
from threading import Timer
import subprocess
from framework import S3PyCliTest
from framework import Config
from framework import logit
from s3client_config import S3ClientConfig
from shlex import quote

class AwsTest(S3PyCliTest):
    def __init__(self, description):
        os.environ["AWS_SHARED_CREDENTIALS_FILE"] = os.path.join(os.path.dirname(os.path.realpath(__file__)), Config.aws_shared_credential_file)
        os.environ["AWS_CONFIG_FILE"] = os.path.join(os.path.dirname(os.path.realpath(__file__)), Config.aws_config_file)
        super(AwsTest, self).__init__(description)

    def setup(self):
        if hasattr(self, 'filename') and hasattr(self, 'filesize'):
            file_to_create = os.path.join(self.working_dir, self.filename)
            logit("Creating file [%s] with size [%d]" % (file_to_create, self.filesize))
            with open(file_to_create, 'wb') as fout:
                fout.write(os.urandom(self.filesize))
        super(AwsTest, self).setup()

    def run(self):
        super(AwsTest, self).run()

    def with_cli(self, cmd):
        if Config.no_ssl:
            cmd = cmd + " --endpoint-url %s" % (S3ClientConfig.s3_uri_http)
        else:
            cmd = cmd + " --endpoint-url %s" % (S3ClientConfig.s3_uri_https)
        super(S3PyCliTest, self).with_cli(cmd)

    def teardown(self):
        super(AwsTest, self).teardown()

    def tagset(self, tags):
        tags = json.dumps(tags, sort_keys=True)
        return "{ \"TagSet\": %s }" % (tags)

    def create_bucket(self, bucket_name, region=None, host=None):
        self.bucket_name = bucket_name
        if region:
            self.with_cli("aws s3api" + " create-bucket " + "--bucket " + bucket_name +
                          " --region" + region)
        else:
            self.with_cli("aws s3api" + " create-bucket " + "--bucket " + bucket_name)

        return self

    def put_bucket_tagging(self, bucket_name, tags=None):
        self.bucket_name = bucket_name
        tagset = self.tagset(tags)
        self.with_cli("aws s3api " + " put-bucket-tagging " + "--bucket " + bucket_name
                      + " --tagging " + quote(tagset) )
        return self

    def list_bucket_tagging(self, bucket_name):
        self.bucket_name = bucket_name
        self.with_cli("aws s3api " + "get-bucket-tagging " + "--bucket " + bucket_name)
        return self

    def delete_bucket_tagging(self, bucket_name):
        self.bucket_name = bucket_name
        self.with_cli("aws s3api " + "delete-bucket-tagging " + "--bucket " + bucket_name)
        return self

    def delete_bucket(self, bucket_name):
        self.bucket_name = bucket_name
        self.with_cli("aws s3api " + " delete-bucket " + "--bucket " + bucket_name)
        return self

    def put_object_with_tagging(self, bucket_name,filename, filesize, tags=None):
        self.filename = filename
        self.filesize = filesize
        self.bucket_name = bucket_name
        self.tagset = tags
        self.with_cli("aws s3api " + " put-object " + " --bucket " + bucket_name + " --key " + filename
                      + " --tagging " + quote(self.tagset) )
        return self

    def put_object_tagging(self, bucket_name,filename, tags=None):
        self.filename = filename
        self.bucket_name = bucket_name
        tagset = self.tagset(tags)
        self.with_cli("aws s3api " + " put-object-tagging " + " --bucket " + bucket_name + " --key " + filename
                      + " --tagging " + quote(tagset) )
        return self

    def list_object_tagging(self, bucket_name, object_name):
        self.bucket_name = bucket_name
        self.with_cli("aws s3api " + "get-object-tagging " + " --bucket " + bucket_name + " --key " + object_name)
        return self

    def delete_object_tagging(self, bucket_name, object_name):
        self.bucket_name = bucket_name
        self.with_cli("aws s3api " + "delete-object-tagging " + "--bucket " + bucket_name + " --key " + object_name)
        return self

    def delete_object(self, bucket_name, object_name):
        self.bucket_name = bucket_name
        self.with_cli("aws s3api " + "delete-object " + "--bucket " + bucket_name + " --key " + object_name)
        return self

    def create_multipart_upload(self, bucket_name, filename, filesize, tags=None):
        self.filename = filename
        self.filesize = filesize
        self.bucket_name = bucket_name
        self.tagset = tags
        self.with_cli("aws s3api " + " create-multipart-upload " + " --bucket " + bucket_name + " --key " + filename
                      + " --tagging " + quote(self.tagset) )
        return self

    def upload_part(self, bucket_name, filename, filesize, key_name ,part_number, upload_id):
        self.filename = filename
        self.filesize = filesize
        self.key_name = key_name
        self.bucket_name = bucket_name
        self.part_number=part_number
        self.upload_id=upload_id
        self.with_cli("aws s3api " + " upload-part  " + " --bucket " + bucket_name + " --key " + key_name
                      + " --part-number " + part_number + " --body " + filename + " --upload-id " + upload_id)
        return self

    def complete_multipart_upload(self, bucket_name, key_name, parts, upload_id):
        self.key_name = key_name
        self.bucket_name = bucket_name
        self.parts = parts
        self.upload_id = upload_id
        self.with_cli("aws s3api " + " complete-multipart-upload " + " --multipart-upload " + quote(parts) + " --bucket " + bucket_name + " --key " + key_name
                      + " --upload-id " + upload_id )
        return self

