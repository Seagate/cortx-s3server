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
from cortx.utils.message_bus import MessageBus, MessageProducer, MessageConsumer, MessageBusAdmin

class S3CortxMsgBus:

    _message_bus = None
    def __init__(self):
        """Init."""
        self._producer = None
        self._consumer = None

    def connect(self):
        """Connect to Message Bus."""
        try:
            if S3CortxMsgBus._message_bus is None:
                S3CortxMsgBus._message_bus = MessageBus()
        except Exception as exception:
            msg = ("msg_bus init except:%s %s") % (exception, traceback.format_exc())
            return False, msg
        return True, None

    def setup_producer(self, prod_id, msg_type, method):
        """Setup producer."""
        if not S3CortxMsgBus._message_bus:
            raise Exception("Non Existent Message Bus")
        try:
            self._producer = MessageProducer(S3CortxMsgBus._message_bus, \
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
        if not S3CortxMsgBus._message_bus:
            raise Exception("Non Existent Message Bus, Cannot Purge")
        self._producer.delete()

    def setup_consumer(self, cons_id, group, msg_type, auto_ack, offset):
        """Setup the consumer."""
        if not S3CortxMsgBus._message_bus:
            raise Exception("Non Existent Message Bus")
        try:
            self._consumer = MessageConsumer(S3CortxMsgBus._message_bus, consumer_id=cons_id, \
            consumer_group=group, message_types=[msg_type], auto_ack=auto_ack, offset=offset)
        except Exception as exception:
            msg = ("msg_bus setup_consumer except:%s %s") % (
                exception, traceback.format_exc())
            return False, msg
        return True, None

    def receive(self, daemon_mode):
        """Receive the incoming message."""
        try:
            if daemon_mode:
                #timeout=0 makes it as blocking indefinitely
                message = self._consumer.receive(timeout=0)
            else:
                #timeout = 0.5 sec, by default, which is non-blocking
                message = self._consumer.receive()
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

    def count(self, consumer_group):
        """Get the count of unread messages."""
        unread_count = 0
        try:
            unread_count = self._producer.get_unread_count(consumer_group)
        except:
            return 0
        return unread_count

    @staticmethod
    def create_topic(admin_id: str, message_types: list, partitions: int):
        """create topic."""
        try:
            if S3CortxMsgBus._message_bus:
                mbadmin = MessageBusAdmin(S3CortxMsgBus._message_bus, admin_id)
                mbadmin.register_message_type(message_types = message_types,
                                            partitions = partitions)
        except Exception as e:
            raise Exception("Failed to create topic")

    @staticmethod
    def increase_partitions(admin_id: str, message_types: list, partitions: int):
        """Increase partition count for given topic."""
        try:
            if S3CortxMsgBus._message_bus:
                mbadmin = MessageBusAdmin(S3CortxMsgBus._message_bus, admin_id)
                mbadmin.increase_parallelism(message_types = message_types, partitions = partitions)
        except Exception as e:
            raise Exception("Failed to increase partition")

    @staticmethod
    def delete_topic(admin_id: str, message_types: list):
        """Delete given topic"""
        try:
            if S3CortxMsgBus._message_bus:
                mbadmin = MessageBusAdmin(S3CortxMsgBus._message_bus,
                                        admin_id)
                mbadmin.deregister_message_type(message_types = message_types)
        except Exception as e:
            raise Exception("Failed to delete topic")

    @staticmethod
    def list_topics(admin_id: str):
        """list all available topics"""
        if S3CortxMsgBus._message_bus:
            mbadmin = MessageBusAdmin(S3CortxMsgBus._message_bus, admin_id)
            return mbadmin.list_message_types()

    @staticmethod
    def is_topic_exist(admin_id: str, topic_name: str):
        """retuns true if topic exist else false"""
        if S3CortxMsgBus._message_bus:
            mbadmin = MessageBusAdmin(S3CortxMsgBus._message_bus, admin_id)
            if topic_name in mbadmin.list_message_types():
                return True
            return False

