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

"""Implementation of MessageBus for object recovery."""

import json
import socket
import time
import traceback
from s3msgbus.cortx_s3_msgbus import S3CortxMsgBus
from s3backgrounddelete.object_recovery_validator import ObjectRecoveryValidator

class ObjectRecoveryMsgbus(object):

    """This class is implementation of msgbus for object recovery."""

    def __init__(self, config, logger):
        """Initialize MessageBus."""
        self._config = config
        self._logger = logger
        self.__msgbuslib = S3CortxMsgBus()
        self.__isproducersetupcomplete = False
        self.__isconsumersetupcomplete = False
        self._daemon_mode = config.get_daemon_mode()
        self._sleep_time = config.get_msgbus_consumer_sleep_time()

    def close(self):
        """Closure and cleanup for ObjectRecoveryMsgbus."""
        pass

    def __process_msg(self, msg):
        """Loads the json message and sends it to validation and processing."""
        self._logger.info(
            "Processing following records in consumer: " + msg)
        try:
            # msg: {"Key": "egZPBQAAAAA=-ZQIAAAAAJKc=",
            #       "Value": "{\\"index_id\\":\\"egZPBQAAAHg=-YwIAAAAAJKc=\\",
            #       \\"object_layout_id\\":1,
            #        \\"object_metadata_path\\":\\"object1\\"}\\n"}
            probable_delete_records = json.loads(msg)

            if probable_delete_records:
                validator = ObjectRecoveryValidator(
                    self._config, probable_delete_records, self._logger)
                validator.process_results()

        except (KeyError, ValueError) as ex:    # Bad formatted message. Will discard it
            self._logger.error("Failed to parse JSON data due to: " + str(ex))
            return True
        except Exception as ex:
            self._logger.error(str(ex))
            return False
        return True

    def __setup_consumer(self,
        consumer_id = None,
        consumer_group = None,
        msg_topic = None,
        offset = None):
        """Setup steps required for consumer to start receiving."""
        try:
            if not self.__msgbuslib:
                self._logger.error("__msgbuslib is not initialized")
                self.__isconsumersetupcomplete = False
                return

            #Over here we will have msgbuslib loaded
            if not consumer_id:
                consumer_id = self._config.get_msgbus_consumer_id_prefix() + str(socket.gethostname())
            
            if not consumer_group:
                consumer_group = self._config.get_msgbus_consumer_group()
            
            if not msg_topic:
                msg_topic = self._config.get_msgbus_topic()

            if not offset:
                offset = 'earliest'

            self._logger.debug("Setting up S3MessageBus for consumer")
            ret, msg = self.__msgbuslib.setup_consumer(consumer_id,
                consumer_group, msg_topic, False, offset)
            if not ret:
                self._logger.error("Failed to setup message bus for consumer: " + str(msg))
                self.__isconsumersetupcomplete = False
            else:
                self._logger.debug("setup message bus for consumer success")
                self.__isconsumersetupcomplete = True
        except Exception as exception:
            self._logger.error("Exception:{}".format(exception))
            self.__isconsumersetupcomplete = False

    def receive_data(self,
        consumer_id = None,
        consumer_group = None,
        msg_topic = None,
        offset = None):
        """Initializes consumer, connects and receives messages from message bus."""
        while True:
            try:
                if not self.__isconsumersetupcomplete:
                    self.__setup_consumer(consumer_id,
                        consumer_group, msg_topic, offset)
                    if not self.__isconsumersetupcomplete:
                        if not self._daemon_mode:
                            self._logger.debug("Not launched in daemon mode, so exitting.")
                            break
                        time.sleep(self._sleep_time)
                        continue
                #Over here we can assume consumer is set up.
                while True:
                    # We will keep on receiving until there are messages to receive.
                    # Failure is treated as no message in queue, in that case we sleep
                    # for a specified duration and then try to receive again.
                    # In case of non-daemon mode we will exit once we encounter failure
                    # in receiving messages.
                    self._logger.debug("Receiving msg from S3MessageBus")
                    ret,message = self.__msgbuslib.receive(self._daemon_mode)
                    if ret:
                        # Process message can fail, but we still acknowledge the message
                        # The last step in process message is to delete the entry from
                        # probable delete index. Even if we acknowledge a message that
                        # has failed being processed it would eventually come back as
                        # the entry has not been deleted from probable delete index.
                        self._logger.debug("Msg {}".format(str(message)))
                        self.__process_msg(message.decode('utf-8'))
                        self.__msgbuslib.ack()
                    else:
                        self._logger.debug("Failed to receive msg from message bus : " + str(message))
                        if not self._daemon_mode:
                            break
                        #It works with/without sleep time but
                        #In case of multiple exceptions, cpu utilization will be very high
                        time.sleep(self._sleep_time)
            except Exception as exception:
                self._logger.error("Receive Data Exception : {} {}".format(exception, traceback.format_exc()))
                self.__isconsumersetupcomplete = False                
            finally:
                time.sleep(self._sleep_time)
            
            if not self._daemon_mode:
                break

    def __setup_producer(self,
        producer_id = None,
        msg_type = None,
        delivery_mechanism = None):
        """Setup steps required for producer to start sending."""
        try:
            if not self.__msgbuslib:
                self._logger.error("__msgbuslib is not initialized")
                self.__isproducersetupcomplete = False
                return

            if not producer_id:
                producer_id = self._config.get_msgbus_producer_id()
            
            if not msg_type:
                msg_type = self._config.get_msgbus_topic()
            
            if not delivery_mechanism:
                delivery_mechanism = self._config.get_msgbus_producer_delivery_mechanism()

            self._logger.debug("producer id : " + producer_id +  "msg_type : " + str(msg_type) +  "delivery_mechanism : "+ str(delivery_mechanism) )
            ret,msg = self.__msgbuslib.setup_producer(producer_id, msg_type, delivery_mechanism)
            if not ret:
                self._logger.error("setup_producer failed {}".format(str(msg)))
                self.__isproducersetupcomplete = False
            else:
                self.__isproducersetupcomplete = True

        except Exception as exception:
            self._logger.error("Exception:{}".format(exception))
            self.__isproducersetupcomplete = False

    def send_data(self, data,
        producer_id = None,
        msg_type = None,
        delivery_mechanism = None):
        """Send message data."""
        self._logger.debug("In send_data")

        try:
            if not self.__isproducersetupcomplete:
                self.__setup_producer(producer_id,
                                      msg_type,
                                      delivery_mechanism)
                if not self.__isproducersetupcomplete:
                    self._logger.debug("send_data producer connection issues")
                    return False

            msgbody = json.dumps(data)
            self._logger.debug("MsgBody : {}".format(msgbody))
            return self.__msgbuslib.send([msgbody])

        except Exception as exception:
            self._logger.error("Exception:{}".format(exception))
            self.__isproducersetupcomplete = False
            return False

    def purge(self):
        """Purge the messages."""
        self._logger.debug("In Purge")

        try:
            if not self.__isproducersetupcomplete:
                self.__setup_producer()
                if not self.__isproducersetupcomplete:
                    self._logger.debug("purge producer connection issues")
                    return False
            self.__msgbuslib.purge()
            #Insert a delay of 1 min (default) after purge, so that the messages are deleted
            time.sleep(self._config.get_purge_sleep_time())
            self._logger.debug("Purged Messages")
        except Exception as exception:
            self._logger.error("Exception:{}".format(exception))
            self.__isproducersetupcomplete = False
            return False
            
    def get_count(self):
        """Get count of unread messages."""
        self._logger.debug("In get_count")
        
        try:
            consumer_group = self._config.get_msgbus_consumer_group()
            if not self.__isproducersetupcomplete:
                self.__setup_producer()
                if not self.__isproducersetupcomplete:
                    self._logger.debug("get_count producer connection issues")
                    return 0
            
            unread_count = self.__msgbuslib.count(consumer_group)
            if unread_count is None:
                return 0
            else:
                return unread_count
        except Exception as exception:
            self._logger.error("Exception:{}".format(exception))
            return 0
