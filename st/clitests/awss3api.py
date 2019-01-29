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
