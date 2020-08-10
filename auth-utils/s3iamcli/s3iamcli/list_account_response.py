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
from collections import OrderedDict
from s3iamcli.authserver_response import AuthServerResponse

class ListAccountResponse(AuthServerResponse):

    # Printing account info while listing users.
    def print_account_listing(self):

        if self.accounts is None:
            print("No accounts found.")
            return

        account_members = None

        try:
            account_members = self.accounts['member']
        except KeyError:
            logging.exception('Failed to list accounts')
            self.is_valid = False

        # For only one account, account_members will be of type OrderedDict, convert it to list type
        if type(account_members) == OrderedDict:
            self.print_listing_info(account_members)
        else:
            for account in account_members:
                self.print_listing_info(account)

    def print_listing_info(self, account):

        message = ("AccountName = %s, AccountId = %s, CanonicalId = %s, Email = %s" %
              (self.get_value(account, 'AccountName'), self.get_value(account, 'AccountId'),
               self.get_value(account, 'CanonicalId'), self.get_value(account, 'Email')))
        print(message)

    # Validator for list account operation
    def validate_response(self):

        super(ListAccountResponse, self).validate_response()
        self.accounts = None
        try:
            self.accounts = (self.response_dict['ListAccountsResponse']['ListAccountsResult']
                             ['Accounts'])
        except KeyError:
            self.is_valid = False
            logging.exception('Failed to list accounts from account response')
