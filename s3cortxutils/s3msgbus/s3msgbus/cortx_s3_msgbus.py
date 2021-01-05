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

""" This class stores the message bus wrapper interface """

#!/usr/bin/python3.6

import os.path
import errno
import traceback
from cortx.utils.message_bus import MessageBus, MessageProducer, MessageConsumer

class S3CortxMsgBus:

    def __init__(self):
        """ Initialize the message bus """
        self._message_bus = None

    def connect(self):
        """ Try to connect to the message bus """
        try:
            self._message_bus = MessageBus()
        except Exception as exception:
            msg = ("msg_bus init except:%s %s") % (exception, traceback.format_exc())
            return False, msg
        return True, None

    def setup_send(self, id, msg_type, method):
        """ Setup for Message Send """
        if not self._message_bus:
            raise Exception("Non Existent Message Bus")
        try:
            self.producer = MessageProducer(self._message_bus, \
            producer_id=id , message_type = msg_type, method = method)
        except Exception as exception:
             msg = ("msg_bus setup except:%s %s") % (
                exception, traceback.format_exc())
             return False, msg
        return True, None

    def send(self, messages):
        """ Send the constructed message """
        try:
            self.producer.send(messages)
        except Exception as exception:
            msg = ("msg_bus send except:%s %s") % (
                exception, traceback.format_exc())
            return False, msg
        return True, None

    def setup_receive(self, id, group, msg_type, auto_ack, offset):
        """ Setup for message receive """
        if not self._message_bus:
            raise Exception("Non Existent Message Bus")
        try:
            self.consumer = MessageConsumer(self._message_bus, consumer_id=id, \
            consumer_group=group, message_type= [msg_type], auto_ack=auto_ack, offset=offset)
        except Exception as exception:
            msg = ("msg_bus setup_receive except:%s %s") % (
                exception, traceback.format_exc())
            return False, msg
        return True, None

    def receive(self):
        """ Receive the incoming message """
        try:
            message = self.consumer.receive()
        except Exception as exception:
            msg = ("msg_bus receive except:%s %s") % (
                exception, traceback.format_exc())
            return False, msg
        self.consumer.ack()
        print(message)
        return True, message

    def commit(self):
        """Acknowledges the messages to message bus"""
        self.consumer.ack()

