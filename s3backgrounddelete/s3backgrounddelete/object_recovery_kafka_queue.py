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

"""Implementation of Kafka for producer."""
import json
from confluent_kafka import Producer
import confluent_kafka as kafka
import socket

class ObjectRecoveryKafkaProd(object):

    """This class is implementation of Kafka for object recovery."""

    def __init__(self, config, logger):
        """Initialize kafka."""
        self._config = config
        self.prod_config = {'bootstrap.servers': (self._config.get_kafka_host() + ':' +  str(self._config.get_kafka_port())), 'client.id': socket.gethostname()}
        self.logger = logger
        self.__producer = Producer(self.prod_config)

    def delivery(self, err, msg):
        if err:
            self.logger.error(
                "Object recovery queue send data "+ str(msg) +
                " failed :" + msg + str(err))
        else:
            self.logger.info(
                "Object recovery queue send data successfully : key: " +
                str(msg.key()) + " value:" + str(msg.value()))
            self.logger.info("offset:" + str(msg.offset()) + " partition:" + str(msg.partition()))

    def send_data(self, data):
        """Send message data."""
        self.__producer.poll(0)
        try:
            self.__producer.produce(self._config.get_kafka_topic(), json.dumps(data), callback=self.delivery)
        except BufferError as buffer_error:
            self.logger.error(
                "BufferError:{}".format(buffer_error.str()))
            return False

        except kafka.KafkaException as kafka_exception:
            self.logger.error("KafkaException:{}".format(kafka_exception.str()))
            return False
         
        return True

    def close(self):
        """Block until all messages are delivered/failed."""
        self.__producer.flush()
