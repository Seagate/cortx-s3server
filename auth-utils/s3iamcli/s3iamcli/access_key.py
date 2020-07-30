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

class AccessKey:
    def __init__(self, iam_client, cli_args):
        self.iam_client = iam_client
        self.cli_args = cli_args

    def create(self):
        access_key_args = {}
        if(not self.cli_args.name is None):
            access_key_args['UserName'] = self.cli_args.name

        try:
            result = self.iam_client.create_access_key(**access_key_args)
        except Exception as ex:
            print("Failed to create access key.")
            print(str(ex))
            return

        print("AccessKeyId = %s, SecretAccessKey = %s, Status = %s" % (
                result['AccessKey']['AccessKeyId'],
                result['AccessKey']['SecretAccessKey'],
                result['AccessKey']['Status']))

    def delete(self):
        if(self.cli_args.access_key_update is None):
            print("Access Key id is required.")
            return

        access_key_args = {}
        access_key_args['AccessKeyId'] = self.cli_args.access_key_update

        if(not self.cli_args.name is None):
            access_key_args['UserName'] = self.cli_args.name

        try:
            self.iam_client.delete_access_key(**access_key_args)
        except Exception as ex:
            print("Failed to delete access key.")
            print(str(ex))
            return

        print("Access key deleted.")

    def update(self):
        if(self.cli_args.access_key_update is None):
            print("Access Key id is required.")
            return

        if(self.cli_args.status is None):
            print("Access Key status is required.")
            return

        access_key_args = {}
        access_key_args['AccessKeyId'] = self.cli_args.access_key_update
        access_key_args['Status'] = self.cli_args.status

        if(not self.cli_args.name is None):
            access_key_args['UserName'] = self.cli_args.name

        try:
            self.iam_client.update_access_key(**access_key_args)
        except Exception as ex:
            print("Failed to update access key.")
            print(str(ex))
            return

        print("Access key Updated.")

    def list(self):
        access_key_args = {}
        if(not self.cli_args.name is None):
            access_key_args['UserName'] = self.cli_args.name

        try:
            result = self.iam_client.list_access_keys(**access_key_args)
        except Exception as ex:
            print("Failed to list access keys.")
            print(str(ex))
            return

        for accesskey in result['AccessKeyMetadata']:
            print("UserName = %s, AccessKeyId = %s, Status = %s" % (accesskey['UserName'],
                        accesskey['AccessKeyId'], accesskey['Status']))
