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

import traceback
from cortx.utils.iem_framework import EventMessage


class S3CortxIem:

    def __init__(self):
        """Init."""

    def send(self, module, event_id, severity, message_blob):
        """Send the message."""
        source = 'S'
        component= 'S3' 
        EventMessage.init(component, source)

        try:
            EventMessage.send(module=module, event_id=event_id, severity=severity, message_blob=message_blob)
        except Exception as exception:
            msg = ("msg_bus ack except:%s %s") % (
                    exception, traceback.format_exc())
            return False, msg
        return True, None


    def receive(self, component):
        """Receive the incoming message."""
        EventMessage.subscribe(component)

        while True:
            try: 
                alert = EventMessage.receive()
                if alert is None:
                    break
            except Exception as exception: 
                msg = ("msg_bus ack except:%s %s") % (
                    exception, traceback.format_exc())
                return False, msg
        return True, None
