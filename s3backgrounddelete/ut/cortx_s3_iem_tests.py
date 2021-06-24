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

from s3backgrounddelete.IEMutil import IEMutil

def test_send():
    s3iem = IEMutil("HPI", "INFO", 100, "Test")
    severity = 'X'
    module = 'HPI'
    event_id = 100
    message_blob = "*******This is a test IEM message********"

    result_data = s3iem.send(module, event_id, severity, message_blob)
    assert result_data[0] == True

def test_receive():
    s3iem = IEMutil("HPI", "INFO", 100, "Test")
    component = 'S3'
    severity = 'X'
    module = 'HPI'
    event_id = 100
    message_blob = "Test message"

    ret, msg = s3iem.send(module, event_id, severity, message_blob)

    result_data = s3iem.receive(component)
    assert result_data[0] == True
    assert result_data[1] == None 
