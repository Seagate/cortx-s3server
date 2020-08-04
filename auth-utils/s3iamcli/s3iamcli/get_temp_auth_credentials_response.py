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

class GetTempAuthCredentialsResponse(AuthServerResponse):

    # Printing credentials
    def print_credentials(self):
        print("AccessKeyId = %s, SecretAccessKey = %s, ExpiryTime = %s, SessionToken = %s" %
              (self.get_value(self.credentials, 'AccessKeyId'),
               self.get_value(self.credentials, 'SecretAccessKey'),
               self.get_value(self.credentials, 'ExpiryTime'),
               self.get_value(self.credentials, 'SessionToken')
               ))

    # Validator for create account operation
    def validate_response(self):
        super(GetTempAuthCredentialsResponse, self).validate_response()
        self.credentials = None
        try:
            self.credentials = (self.response_dict['GetTempAuthCredentialsResponse']['GetTempAuthCredentialsResult']['AccessKey'])
        except KeyError:
            logging.exception('Failed to obtain credentials from GetTempAuthCredentialsResponse')
            self.is_valid = False

