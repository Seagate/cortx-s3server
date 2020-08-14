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

"""This will generate error response"""


class CORTXS3ErrorResponse(object):
    """Generate error message and reason."""
    _http_status = -1
    _error_message = ""
    _error_reason = ""

    def __init__(self, http_status_code, error_reason, error_message):
        """Initialise and parse error response."""
        self._http_status = http_status_code
        self._error_reason = error_reason
        self._error_message = error_message

    def get_error_status(self):
        """Return error status."""
        return self._http_status

    def get_error_message(self):
        """Return error message."""
        return self._error_message

    def get_error_reason(self):
        """Return error reason."""
        return self._error_reason
