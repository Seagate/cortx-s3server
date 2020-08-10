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
from s3iamcli.cli_response import CLIResponse

class Role:
    def __init__(self, iam_client, cli_args):
        self.iam_client = iam_client
        self.cli_args = cli_args

    def create(self):
        if(self.cli_args.file is None):
            message = "Assume role policy document is required to create role."
            CLIResponse.send_error_out(message)

        if(self.cli_args.name is None):
            print("RoleName is required to create role.")
            CLIResponse.send_error_out(message)

        file_path = os.path.abspath(self.cli_args.file)
        if(not os.path.isfile(file_path)):
            message = "File " + file_path + " not found."
            CLIResponse.send_error_out(message)

        with open (file_path, "r") as assume_role_file:
            assume_role_policy = assume_role_file.read()

        role_args = {}
        role_args['AssumeRolePolicyDocument'] = assume_role_policy
        role_args['RoleName'] = self.cli_args.name

        if(not self.cli_args.path is None):
            role_args['Path'] = self.cli_args.path

        try:
            result = self.iam_client.create_role(**role_args)
        except Exception as ex:
            message = "Exception occured while creating role."
            message += str(ex)
            CLIResponse.send_error_out(message)

        print("RoleId = %s, RoleName = %s, ARN = %s, Path = %s" % (result['Role']['RoleId'],
                    result['Role']['RoleName'], result['Role']['Arn'], result['Role']['Path']))

    def delete(self):
        if(self.cli_args.name is None):
            print("Role name is required to delete role.")
            CLIResponse.send_error_out(message)

        role_args = {}
        role_args['RoleName'] = self.cli_args.name

        try:
            self.iam_client.delete_role(**role_args)
        except Exception as ex:
            message = "Exception occured while deleting role."
            message += str(ex)
            CLIResponse.send_error_out(message)

        message = "Role deleted."
        CLIResponse.send_success_out(message)

    def list(self):
        role_args = {}
        if(not self.cli_args.path is None):
            role_args['PathPrefix'] = self.cli_args.path

        try:
            result = self.iam_client.list_roles(**role_args)
        except Exception as ex:
            message = "Exception occured while listing roles.\n"
            message += str(ex)
            CLIResponse.send_error_out(message)

        for role in result['Roles']:
            print("RoleId = %s, RoleName = %s, ARN = %s, Path = %s" % (role['RoleId'],
                        role['RoleName'], role['Arn'], role['Path']))
