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
        self.credentials = ""
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

    def add_headers(self, headers=None):
        add_header_option = ""
        if headers and len(headers):
            for key, val in headers.items():
                add_header_option +=  " --{} {}".format(key.lower(),val)
        self.command = self.command + add_header_option
        return self

    def with_cli(self, cmd):
        if cmd.startswith("curl"):
           cmd = cmd
        elif Config.no_ssl:
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

    def put_bucket_canned_acl(self, bucket_name, canned_acl):
        self.bucket_name = bucket_name
        self.with_cli("aws s3api" + " create-bucket " + "--bucket " + bucket_name
                      + " --acl " + canned_acl )
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

    def list_objects(self, bucket_name, max_keys=None, max_items=None):
        self.bucket_name = bucket_name
        cmd = "aws s3api " + "list-objects " + "--bucket " + bucket_name
        if(max_keys is not None):
           self.max_keys = max_keys
           cmd = cmd + " --max-keys " + max_keys

        if(max_items is not None):
           cmd = cmd + " --max-items " + max_items

        self.with_cli(cmd)
        return self

    def list_objects_v2(self, bucket_name, **kwargs_options):
        self.bucket_name = bucket_name
        cmd = "aws s3api " + "list-objects-v2 " + "--bucket " + bucket_name
        if("prefix" in kwargs_options.keys()):
            cmd = cmd + " --prefix " + kwargs_options['prefix']
        if("delimiter" in kwargs_options.keys()):
            cmd = cmd + " --delimiter " + kwargs_options['delimiter']
        if("page-size" in kwargs_options.keys()):
           self.max_keys = kwargs_options['page-size']
           cmd = cmd + " --page-size " + str(self.max_keys)
        if("start-after" in kwargs_options.keys()):
            cmd = cmd + " --start-after " + kwargs_options['start-after']
        if("starting-token" in kwargs_options.keys()):
            cmd = cmd + " --starting-token " + kwargs_options['starting-token']
        if("max-items" in kwargs_options.keys()):
            cmd = cmd + " --max-items " + str(kwargs_options['max-items'])

        self.with_cli(cmd)
        return self

    def list_objects_prefix_delimiter(self, bucket_name, max_keys=None, prefix=None, delimiter=None):
        self.bucket_name = bucket_name
        cmd = "aws s3api " + "list-objects " + "--bucket " + bucket_name
        if(max_keys is not None):
           self.max_keys = max_keys
           cmd = cmd + " --max-keys " + max_keys
        if(delimiter is not None):
           cmd = cmd + " --delimiter " + delimiter
        if(prefix is not None):
           cmd = cmd + " --prefix " + prefix
        self.with_cli(cmd)
        return self

    def delete_object_tagging(self, bucket_name, object_name):
        self.bucket_name = bucket_name
        self.with_cli("aws s3api " + "delete-object-tagging " + "--bucket " + bucket_name + " --key " + object_name)
        return self

    def delete_object(self, bucket_name, object_name):
        self.bucket_name = bucket_name
        self.with_cli("aws s3api " + "delete-object " + "--bucket " + bucket_name + " --key " + object_name)
        self.command = self.command + self.credentials
        return self

    def delete_multiple_objects(self, bucket_name, object_list_file):
        self.bucket_name = bucket_name
        self.with_cli("aws s3api " + "delete-objects " + "--bucket " + bucket_name + " --delete file://" + object_list_file)
        self.command = self.command + self.credentials
        return self

    def create_multipart_upload(self, bucket_name, filename, filesize, tags=None, debug_flag=None):
        self.filename = filename
        self.filesize = filesize
        self.bucket_name = bucket_name
        self.tagset = tags
        self.with_cli("aws s3api " + " create-multipart-upload " + " --bucket " + bucket_name + " --key " + filename
                      + " --tagging " + quote(self.tagset) )
        if(debug_flag is not None):
           self.command = self.command + " --debug"
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

    def abort_multipart_upload(self, bucket_name, key_name, upload_id):
        self.key_name = key_name
        self.bucket_name = bucket_name
        self.upload_id = upload_id
        self.with_cli("aws s3api " + " abort-multipart-upload " + " --bucket " + bucket_name + " --key " + key_name
                      + " --upload-id " + upload_id )
        return self

    def put_object(self, bucket_name, filename, filesize=None, canned_acl=None, debug_flag=None, key_name=None):
        self.bucket_name = bucket_name
        self.filename = filename
        if(key_name is not None):
            cmd = "aws s3api " + "put-object " + "--bucket " + bucket_name + " --key " + key_name
        else:
            cmd = "aws s3api " + "put-object " + "--bucket " + bucket_name + " --key " + filename

        if(filesize is not None):
           self.filesize = filesize
           cmd = cmd + " --body "+ self.filename
        if(canned_acl is not None):
           self.canned_acl = canned_acl
           cmd = cmd + " --acl " + canned_acl
        if(debug_flag is not None):
           cmd = cmd + " --debug"
        self.with_cli(cmd)
        return self

    def get_object_acl(self, bucket_name, object_name):
        self.bucket_name = bucket_name
        self.object_name = object_name
        self.with_cli("aws s3api " + "get-object-acl " + "--bucket " + bucket_name + " --key " + object_name + " --output " + "json ")
        return self

    def get_bucket_acl(self, bucket_name):
        self.bucket_name = bucket_name
        self.with_cli("aws s3api " + "get-bucket-acl " + "--bucket " + bucket_name + " --output " + "json ")
        return self

    def with_credentials(self, access_key, secret_key):
        self.credentials = " --access_key=" + access_key +\
                           " --secret_key=" + secret_key
        return self

    def put_object_with_permission_headers(self, bucket_name, object_name, permission_header, permission_header_value):
        cmd = "aws s3api " + "put-object " + "--bucket " + bucket_name + " --key " + object_name +  " --" + permission_header + " " + permission_header_value
        self.with_cli(cmd)
        return self

    def create_bucket_with_permission_headers(self, bucket_name, permission_header, permission_header_value):
        cmd = "aws s3api" + " create-bucket " + "--bucket " + bucket_name +  " --" + permission_header + " " + permission_header_value
        self.with_cli(cmd)
        return self

    def put_object_acl(self, bucket_name, object_name,permission_header, permission_header_value):
        cmd = "aws s3api " + "put-object-acl " + "--bucket " + bucket_name + " --key " + object_name +  " --" + permission_header + " " + permission_header_value
        self.with_cli(cmd)
        return self

    def put_object_acl_with_acp_file(self, bucket_name, object_name, acp):
        cmd = "aws s3api " + "put-object-acl " + "--bucket " + bucket_name + " --key " + object_name +  " --access-control-policy " + acp
        self.with_cli(cmd)
        return self

    def put_acl_with_multiple_options(self, cmd):
        self.with_cli(cmd)
        return self

    def put_object_acl_with_canned_input(self, bucket_name, object_name, canned_input):
        cmd = "aws s3api " + "put-object-acl " + "--bucket " + bucket_name + " --key " + object_name +  " --acl " + canned_input
        self.with_cli(cmd)
        return self

    def put_bucket_acl(self, bucket_name, permission_header, permission_header_value):
        cmd = "aws s3api " + "put-bucket-acl " + "--bucket " + bucket_name +  " --" + permission_header + " " + permission_header_value
        self.with_cli(cmd)
        return self

    def put_bucket_acl_with_canned_input(self, bucket_name, canned_input):
        cmd = "aws s3api " + "put-bucket-acl " + "--bucket " + bucket_name +  " --acl " + canned_input
        self.with_cli(cmd)
        return self

    def put_bucket_acl_with_acp_file(self, bucket_name, acp):
        cmd = "aws s3api " + "put-bucket-acl " + "--bucket " + bucket_name + " --access-control-policy " + acp
        self.with_cli(cmd)
        return self

    def get_object(self, bucket_name, object_name, outfile=None, debug_flag=None):
        self.bucket_name = bucket_name
        self.object_name = object_name
        if outfile is None:
            outfile = object_name
        cmd = "aws s3api get-object --bucket " + bucket_name + " --key " + object_name + " " + outfile
        self.with_cli(cmd)
        if debug_flag is not None:
           self.command += " --debug"
        return self

    def head_object(self, bucket_name, object_name):
        self.bucket_name = bucket_name
        self.object_name = object_name
        self.with_cli("aws s3api " + "head-object " + "--bucket " + bucket_name + " --key " + object_name)
        return self

    def head_bucket(self, bucket_name):
        self.bucket_name = bucket_name
        self.with_cli("aws s3api " + "head-bucket " + "--bucket " + bucket_name)
        return self

    def execute_curl(self, cmd):
        self.with_cli(cmd)
        return self

    def put_bucket_policy(self, bucket_name, policy):
        cmd = "aws s3api " + "put-bucket-policy " + "--bucket " + bucket_name + " --policy " + policy
        self.with_cli(cmd)
        return self

    def get_bucket_policy(self, bucket_name):
        cmd = "aws s3api " + "get-bucket-policy " + "--bucket " + bucket_name
        self.with_cli(cmd)
        return self

    def delete_bucket_policy(self, bucket_name):
        cmd = "aws s3api " + "delete-bucket-policy " + "--bucket " + bucket_name
        self.with_cli(cmd)
        return self
