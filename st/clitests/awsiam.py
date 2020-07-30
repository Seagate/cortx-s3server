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
from framework import PyCliTest
from framework import Config
from framework import logit
from s3client_config import S3ClientConfig
from shlex import quote


class AwsIamTest(PyCliTest):
    def __init__(self, description):
        os.environ["AWS_SHARED_CREDENTIALS_FILE"] = os.path.join(os.path.dirname(os.path.realpath(__file__)), Config.aws_iam_shared_credential_file)
        os.environ["AWS_CONFIG_FILE"] = os.path.join(os.path.dirname(os.path.realpath(__file__)), Config.aws_iam_config_file)
        super(AwsIamTest, self).__init__(description)

    def setup(self):
        if hasattr(self, 'filename') and hasattr(self, 'filesize'):
            file_to_create = os.path.join(self.working_dir, self.filename)
            logit("Creating file [%s] with size [%d]" % (file_to_create, self.filesize))
            with open(file_to_create, 'wb') as fout:
                fout.write(os.urandom(self.filesize))
        super(AwsIamTest, self).setup()

    def run(self):
        super(AwsIamTest, self).run()

    def with_cli(self, cmd):
        cmd = cmd + " --endpoint-url %s" % (S3ClientConfig.iam_uri_https)
        super(AwsIamTest, self).with_cli(cmd)

    def teardown(self):
        super(AwsIamTest, self).teardown()

    def tagset(self, tags):
        tags = json.dumps(tags, sort_keys=True)
        return "{ \"TagSet\": %s }" % (tags)



    def delete_user(self, user_name):
        self.with_cli("aws iam delete-user " + "--user-name " + user_name)
        return self

    def create_user(self, user_name):
        self.with_cli("aws iam create-user " + "--user-name " + user_name)
        return self

    def create_login_profile(self, user_name, password):
        self.with_cli("aws iam create-login-profile " + "--user-name " + user_name + " --password " + password)
        return self

    def update_login_profile(self, user_name):
        self.with_cli("aws iam " + "update-login-profile " + "--user-name " + user_name)
        return self

    def update_login_profile_with_optional_arguments(self, user_name , password ,password_reset_required, no_password_reset_required):
        cmd = "aws iam update-login-profile --user-name "+ user_name
        if(password is not None):
            cmd += " --password %s" % password
        if(password_reset_required is not None):
            cmd += " --password-reset-required"
        elif(no_password_reset_required is not None):
            cmd += " --no-password-reset-required"
        self.with_cli(cmd)
        return self

    def get_login_profile(self, user_name):
        self.user_name = user_name
        self.with_cli("aws iam " + "get-login-profile " + "--user-name " + user_name)
        return self
