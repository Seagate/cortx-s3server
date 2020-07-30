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
import datetime
from datetime import timezone

class CreateAccountLoginProfileResponse(AuthServerResponse):

    # Printing account info while creating user.
    def print_profile_info(self):
        datestr = self.get_value(self.profile, 'CreateDate')
        date_time_obj = datetime.datetime.strptime(datestr, '%Y%m%d%H%M%SZ')
        createdate = date_time_obj.replace(tzinfo=timezone.utc).isoformat()
        print("Account Login Profile: CreateDate = %s, PasswordResetRequired = %s, AccountName = %s" %
              (createdate,
               self.get_value(self.profile, 'PasswordResetRequired'),
               self.get_value(self.profile, 'AccountName')))

    # Validator for create account login profile operation
    def validate_response(self):

        super(CreateAccountLoginProfileResponse, self).validate_response()
        self.profile = None
        try:
            self.profile = (self.response_dict['CreateAccountLoginProfileResponse']['CreateAccountLoginProfileResult']
                            ['LoginProfile'])
        except KeyError:
            logging.exception('Failed to obtain ... from account login profile response')

            self.is_valid = False
