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
import socket

try:
    from cortx.utils.log import Log
    from cortx.utils.message_bus.tcp.kafka.kafka import KafkaProducerChannel
except ImportError:
    pass

class ObjectRecoveryKafkaProd(object):
    """This class is implementation of Kafka for object recovery."""

    def __init__(self, config, logger):
        """Initialize kafka."""
        self._logger = logger

        Log.init('S3_background', config.get_logger_directory())

        self._channel = KafkaProducerChannel(
            hosts=config.get_kafka_hosts(),
            client_id = socket.gethostname())
        self._channel.init()
        self._channel.set_topic(config.get_kafka_topic())

    def send_data(self, data):
        """Send message data."""
        try:
            self._channel.send(json.dumps(data))
            return True
        except BufferError as buffer_error:
            self._logger.error(
                "BufferError: {}".format(buffer_error.str()))
        except Exception as ex:
            self._logger.error("Exception: {}".format(str(ex)))
        return False

    def close(self):
        """Block until all messages are delivered/failed."""
        self._channel.channel().flush()
