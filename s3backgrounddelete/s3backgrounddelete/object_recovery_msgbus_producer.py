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

"""
This class act as object recovery scheduler which add key value to
msgbus topic.
"""
import json
import socket
from s3msgbus.cortx_s3_msgbus import S3CortxMsgBus

class ObjectRecoveryMsgbusProducer(object):
    """This class is implementation of msgbus for object recovery."""
    isMsgBusLoaded = False
    
    def __init__(self, config, logger):
        """Initialize Msgbus."""
        self._config = config
        self.logger = logger
        self.__producer = None

    def __internalconnect(self):
        """Workaround Internal function for connection singleton"""
        self.logger.debug("Instantiating S3MessageBus")
        self.__producer = S3CortxMsgBus()
        self.logger.debug("Connecting to S3MessageBus")
        ret,msg = self.__producer.connect()
        if ret:
            ObjectRecoveryMsgbusProducer.isMsgBusLoaded = True
        else:
            self.logger.error("connect failed {}".format(msg))
            self.__producer = None
        return ret

    def connect(self):
        """Establish connection to message bus"""
        try:
            if not ObjectRecoveryMsgbusProducer.isMsgBusLoaded:
                ret = self.__internalconnect()
            if self.__producer:
                producer_id = self._config.get_msgbus_producer_id()
                msg_type = self._config.get_msgbus_topic()
                delivery_mechanism = self._config.get_msgbus_producer_delivery_mechanism()
                ret,msg = self.__producer.setup_send(producer_id, msg_type, delivery_mechanism)
                if not ret:
                    self.logger.error("setup_send failed")
                    self.__producer = None
            else:
                self.logger.error("connect failed")
                self.__producer = None
        except Exception as exception:
            self.logger.error("Exception:{}".format(exception))

    def send_data(self, data):
        """Send message data."""
        try:
            if not self.__producer:
                self.connect()
                if not self.__producer:
                    self.logger.debug("producer connection issues")
                    return False
            
            msgbody = json.dumps(data)
            self.logger.debug("MsgBody : {}".format(msgbody))
            ret = self.__producer.send([msgbody])
            if not ret:
                return False
            else:
                return True

        except Exception as exception:
            self.logger.error("Exception:{}".format(exception))
            return False

    def close(self):
        """Closure and cleanup for producer."""
        pass

 