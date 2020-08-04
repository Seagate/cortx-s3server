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

class ErrorResponse(AuthServerResponse):

    # Getter for Error Response
    def get_error_message(self):
        return self.error

    # Validator for Error Response
    def validate_response(self):

        super(ErrorResponse, self).validate_response()
        self.error = None
        try:
            self.error =  "An error occurred (" +  self.response_dict['ErrorResponse']['Error']['Code'] + ") : "+ self.response_dict['ErrorResponse']['Error']['Message'];
        except KeyError:
            logging.exception('Failed to obtain error response')
            self.is_valid = False
