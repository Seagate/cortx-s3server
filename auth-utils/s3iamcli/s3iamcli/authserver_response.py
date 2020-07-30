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
import xmltodict
from xml.parsers.expat import ExpatError

class AuthServerResponse():
    def __init__(self, xml_response):
        self.xml_response = xml_response
        self.is_valid = True
        self.response_dict = None
        self.validate_response()

    def get_value(self, ordered_dict, key):
        if ordered_dict.get(key) is not None:
            return ordered_dict[key]
        else:
            logging.debug('Value corresponding to key %s' % key + 'is empty')
            return None

    def validate_response(self):

        self.parse_xml(self.xml_response)
        if not self.is_valid_response():

            logging.exception('Parsing xml returned empty XML')

    def is_valid_response(self):

        return self.is_valid

    def parse_xml(self, xml_response):

        self.response_dict = None
        try:

            self.response_dict = xmltodict.parse(xml_response['body'])

        except (ExpatError, TypeError, KeyError):

            logging.exception('The operation failed with following traceback')

        if self.response_dict is None:
            self.is_valid = False
