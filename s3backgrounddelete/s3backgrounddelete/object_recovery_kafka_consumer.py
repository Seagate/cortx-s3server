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

"""ObjectRecoveryKafka reads messages about objects oid to be deleted from Kafka."""

import json
from cortx.utils.message_bus.tcp.kafka.kafka import KafkaConsumerChannel
from s3backgrounddelete.object_recovery_validator import ObjectRecoveryValidator

class ObjectRecoveryKafkaConsumer:

    """This class reads messages from Kafka and forwards them for handling"""

    def __init__(self, config, logger):
        """Initialize Kafka Consumer."""
        self._config = config
        self._logger = logger

        self._channel = KafkaConsumerChannel(
            hosts = config.get_kafka_hosts(),
            group_id = config.get_kafka_group_id(),
            consumer_name = 's3_background')
        self._channel.init()
        self._channel.channel().subscribe([config.get_kafka_topic()])

    def close(self):
        pass

    def receive_data(self):
        poll_timeout = self._config.get_kafka_poll_timeout()
        daemon_mode = self._config.get_daemon_mode()

        while True:
            msg_list = self._channel.channel().consume(
                num_messages=1,
                timeout=poll_timeout
                )
            msg = msg_list[0] if msg_list else None

            if msg is None:
                if daemon_mode: continue
                else: break
            err = msg.error()
            if err:
                self._logger.error(str(err))
                continue
            if self._treat_msg(msg.value().decode('utf-8')):
                self._channel.acknowledge()

    def _treat_msg(self, msg):
        self._logger.info(
            "Processing following records in consumer: " + msg)
        try:
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
