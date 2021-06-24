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

""" This class generates IEM for background processes
    eventcode part  005 represents s3background component
    and 0001 repesents connection failures """

from cortx.utils.iem_framework import EventMessage

class IEMutil(object):
    severity = ""
    eventCode = ""
    eventstring = ""
    source = 'S'
    component= 'S3'
    _logger = None
    # List of eventcodes
    S3_CONN_FAILURE = "0050010001"

    # List of eventstrings
    S3_CONN_FAILURE_STR = "Failed to connect to S3 server. For more information refer the Troubleshooting Guide."

    def __init__(self, logger, loglevel, eventcode, eventstring):
        self.eventCode = eventcode
        self.loglevel = loglevel
        self.eventString = eventstring
        self._logger = logger
        EventMessage.init(self.component, self.source)
        if(loglevel == "INFO"):
            self.severity = "I"
        if(loglevel == "WARN"):
            self.severity = "W"
        if(loglevel == "ERROR"):
            self.severity = "E"

        self.log_iem()

    def log_iem(self):
        self.send('S3 BG delete', self.eventCode, self.severity, self.eventString)

    @classmethod
    def send(self, module, event_id, severity, message_blob):
        """Send the message."""

        try:
            EventMessage.send(module='S3 BG delete', event_id=event_id, severity=severity, message_blob=message_blob)
        except:
            self._logger.ERROR("Error in sending the IEM message.")
            return False
        return True

    @classmethod
    def receive(self, component):
        """Receive the incoming message."""
        EventMessage.subscribe(component)

        try:
            alert = EventMessage.receive()
        except:
            self._logger.ERROR("Error in receiving the IEM message.")
            return False
        return True
