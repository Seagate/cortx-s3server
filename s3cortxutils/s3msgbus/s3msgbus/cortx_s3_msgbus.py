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
        """Init."""
        self._message_bus = None
        self._producer = None
        self._consumer = None
#        self._blocking = False

    def connect(self):
        """Connect to Message Bus."""
        try:
            self._message_bus = MessageBus()
        except Exception as exception:
            msg = ("msg_bus init except:%s %s") % (exception, traceback.format_exc())
            return False, msg
        return True, None

    def setup_producer(self, prod_id, msg_type, method):
        """Setup producer."""
        if not self._message_bus:
            raise Exception("Non Existent Message Bus")
        try:
            self._producer = MessageProducer(self._message_bus, \
            producer_id=prod_id, message_type=msg_type, method=method)
        except Exception as exception:
             msg = ("msg_bus setup producer except:%s %s") % (
                exception, traceback.format_exc())
             return False, msg
        return True, None

    def send(self, messages):
        """Send the constructed message."""
        try:
            self._producer.send(messages)
        except Exception as exception:
            msg = ("msg_bus send except:%s %s") % (
                exception, traceback.format_exc())
            return False, msg
        return True, None

    def purge(self):
        """Purge/Delete all the messages."""
        if not self._message_bus:
            raise Exception("Non Existent Message Bus, Cannot Purge")
        self._producer.delete()

    def setup_consumer(self, cons_id, group, msg_type, auto_ack, offset):
        """Setup the consumer."""
        if not self._message_bus:
            raise Exception("Non Existent Message Bus")
        try:
            self._consumer = MessageConsumer(self._message_bus, consumer_id=cons_id, \
            consumer_group=group, message_type=[msg_type], auto_ack=auto_ack, offset=offset)
        except Exception as exception:
            msg = ("msg_bus setup_consumer except:%s %s") % (
                exception, traceback.format_exc())
            return False, msg
        return True, None

    def receive(self, mode):
        """Receive the incoming message."""
        try:
            if mode:
                message = self._consumer.receive()
            else:
                message = self._consumer.receive(timeout=0)
        except Exception as exception:
            msg = ("msg_bus receive except:%s %s") % (
                exception, traceback.format_exc())
            return False, msg
        return True, message

    def ack(self):
        """Ack the received message."""
        try:
            self._consumer.ack()
        except Exception as exception:
            msg = ("msg_bus ack except:%s %s") % (
                exception, traceback.format_exc())
            return False, msg
        return True, None
