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

import syslog


class IEMutil(object):
    severity = ""
    eventCode = ""
    eventstring = ""

    # List of eventcodes
    S3_CONN_FAILURE = "0050010001"
    RABBIT_MQ_CONN_FAILURE = "0050020001"

    # List of eventstrings
    S3_CONN_FAILURE_STR = "failed to connect to s3"
    RABBIT_MQ_CONN_FAILURE_STR = "failed to connect to RabbitMq"

    def __init__(self, loglevel, eventcode, eventstring):
        self.eventCode = eventcode
        self.loglevel = loglevel
        self.eventString = eventstring
        if(loglevel == "INFO"):
            self.severity = "I"
        if(loglevel == "WARN"):
            self.severity = "W"
        if(loglevel == "ERROR"):
            self.severity = "E"

        self.log_iem()

    def log_iem(self):
        IEM = "IEC:%sS%s:%s" % (self.severity, self.eventCode, self.eventString)
        syslog.syslog(IEM)
