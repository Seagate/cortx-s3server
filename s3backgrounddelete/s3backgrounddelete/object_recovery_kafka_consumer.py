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
ObjectRecoveryKafka reads messages about objects oid to be deleted from Kafka.
"""
import json
from confluent_kafka import Consumer, KafkaError, KafkaException
from s3backgrounddelete.cortx_s3_config import CORTXS3Config
from s3backgrounddelete.object_recovery_validator import ObjectRecoveryValidator
from s3backgrounddelete.IEMutil import IEMutil


class ObjectRecoveryKafka:
    def __init__(self, config, logger):
        self._config = config
        self._logger = logger
        self._consumer = None
        self._daemon_mode = False if config.get_daemon_mode() == "False" else True

    def close(self):
        if self._consumer is not None:
            self._consumer.close()

    def _treat_msg(self, msg):
        try:
            probable_delete_records = json.loads(msg)
        except Exception as ex:
            self._logger.info(str(ex))
            return True                     # Bad formatted message. Will discard it
        self._logger.info(
            "Processing following records in consumer: " +
            str(probable_delete_records))
        if probable_delete_records is None:
            return True                     # Improbable situation
        try:
            validator = ObjectRecoveryValidator(
                self._config, probable_delete_records, self._logger)
            validator.process_results()
            return True
        except Exception as ex:
            self._logger.error(str(ex))
            return False

    def receive_data(self):
        if not self._consumer:
            consumer_config = {
                'bootstrap.servers': (
                    self._config.get_kafka_host() + ':' +
                    str(self._config.get_kafka_port())),
                'group.id': self._config.get_kafka_group_id(),
                'auto.offset.reset': 'earliest',
                'enable.auto.commit': False
            }
            self._consumer = Consumer(consumer_config)
            self._consumer.subscribe([self._config.get_kafka_topic()])
        poll_timeout = self._config.get_kafka_poll_timeout()

        while True:
            msg = self._consumer.poll(timeout=poll_timeout)
            if msg is None:
                if self._daemon_mode: continue
                else: break
            err = msg.error()
            if err:
                self._logger.error(str(err))
                continue
            if self._treat_msg(msg.value().decode('utf-8')):
                self._consumer.commit(msg)
