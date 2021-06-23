#!/usr/bin/python3.6
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

import mock
import unittest

from s3iem.cortx_s3_iem import S3CortxIem

class S3IemAPIsUT(unittest.TestCase):

    s3iem = S3CortxIem()
    @mock.patch.object(s3iem, 'send')
    def test_mock_send(self, mock_send_return):
        s3iem = S3CortxIem()
        mock_send_return.return_value = True
        severity = 'X'
        module = 'HPI'
        event_id = 100
        message_blob = "*******This is a test IEM message********"

        result_data, msg = s3iem.send(module, event_id, severity, message_blob)
        print(result_data)
        self.assertTrue(result_data == mock_send_return.return_value)

    @mock.patch.object(s3iem, 'receive')
    def test_mock_receive(self, mock_get_return):
        s3iem = S3CortxIem()
        component = 'S3'
        mock_get_return.return_value = True

        result_data, msg = s3iem.receive(component)

        self.assertTrue(result_data == mock_get_return.return_value)
