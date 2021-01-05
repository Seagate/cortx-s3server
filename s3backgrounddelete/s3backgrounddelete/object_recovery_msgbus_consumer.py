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

"""ObjectRecoveryMsgbus reads messages about objects oid to be deleted from MessageBus."""

import json
import socket
import time
import traceback
from logging import FATAL
from s3msgbus.cortx_s3_msgbus import S3CortxMsgBus
from s3backgrounddelete.object_recovery_validator import ObjectRecoveryValidator

class ObjectRecoveryMsgbusConsumer:
    """This class reads messages from MessageBus and forwards them for handling"""

    def __init__(self, config, logger):
        """Initialize MessageBus Consumer."""
        self._config = config
        self._logger = logger
        self._consumer = None
        self._daemon_mode = config.get_daemon_mode()
        self._sleep_time = config.get_msgbus_consumer_sleep_time()

    def close(self):
        pass

    def _process_msg(self, msg):
        """loads the json message and sends it to validation and processing"""
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

    def receive_data(self):
        """Initializes consumer, connects and receives messages from message bus"""
        try:
            self._logger.debug("Instantiating S3MessageBus")
            self._consumer = S3CortxMsgBus()
            self._logger.debug("Connecting to S3MessageBus")
            ret,msg = self._consumer.connect()
            if ret:
                id = self._config.get_msgbus_consumer_id_prefix() + str(socket.gethostname())
                group = self._config.get_msgbus_consumer_group()
                type = self._config.get_msgbus_topic()
                autoack = False
                offset = 'earliest'
                self._logger.debug("Setting up S3MessageBus for consumer")
                ret, msg = self._consumer.setup_receive(id, group, type, autoack, offset)
                if ret:
                    while True:
                        # We will keep on receiving until there are messages to receive.
                        # Failure is treated as no message in queue, in that case we sleep
                        # for a specified duration and then try to receive again.
                        # In case of non-daemon mode we will exit once we encounter failure
                        # in receiving messages.
                        self._logger.debug("Receiving msg from S3MessageBus")
                        ret,message = self._consumer.receive()
                        if ret:
                            # Process message can fail, but we still acknowledge the message
                            # The last step in process message is to delete the entry from
                            # probable delete index. Even if we acknowledge a message that
                            # has failed being processed it would eventually come back as
                            # the entry has not been deleted from probable delete index. 
                            self._process_msg(msg.decode('utf-8'))
                            self._consumer.commit()
                        else:
                            self._logger.debug("Failed to receive msg from message bus : " + str(message))
                            if not self._daemon_mode:
                                break
                            time.sleep(self._sleep_time)
                else:
                    self._logger.error("Failed to setup message bus : " + str(msg))
                    time.sleep(self._sleep_time)
                    self.receive_data()
            else:
                self._logger.error("Failed to connect to message bus : " + str(msg))
                time.sleep(self._sleep_time)
                self.receive_data()
        except Exception as exception:
            self._logger.error("Receive Data Exception : {} {}".format(exception, traceback.format_exc()))

