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

from s3iamcli.cli_response import CLIResponse

class Group:
    def __init__(self, iam_client, cli_args):
        self.iam_client = iam_client
        self.cli_args = cli_args

    def create(self):
        policy_args = {}
        if(self.cli_args.name is None):
            message = "Group name is required."
            CLIResponse.send_error_out(message)

        policy_args['GroupName'] = self.cli_args.name

        if(not self.cli_args.path is None):
            policy_args['Path'] = self.cli_args.path

        try:
            result = self.iam_client.create_group(**policy_args)
        except Exception as ex:
            message = "Exception occured while creating group.\n"
            message += str(ex)
            CLIResponse.send_error_out(message)

        print("GroupId = %s, GroupName = %s, Arn = %s" % (
                result['Group']['GroupId'],
                result['Group']['GroupName'],
                result['Group']['Arn'],
                ))
