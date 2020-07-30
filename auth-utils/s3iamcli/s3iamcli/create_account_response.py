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

import logging
from s3iamcli.authserver_response import AuthServerResponse

class CreateAccountResponse(AuthServerResponse):

    # Printing account info while creating user.
    def print_account_info(self):
        print("AccountId = %s, CanonicalId = %s, RootUserName = %s, AccessKeyId = %s, SecretKey = %s" %
              (self.get_value(self.account, 'AccountId'),
               self.get_value(self.account, 'CanonicalId'),
               self.get_value(self.account, 'RootUserName'),
               self.get_value(self.account, 'AccessKeyId'),
               self.get_value(self.account, 'RootSecretKeyId')))

    # Validator for create account operation
    def validate_response(self):

        super(CreateAccountResponse, self).validate_response()
        self.account = None
        try:
            self.account = (self.response_dict['CreateAccountResponse']['CreateAccountResult']
                            ['Account'])
        except KeyError:
            logging.exception('Failed to obtain create account key from account response')
            self.is_valid = False
