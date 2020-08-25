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

"""Implementation of RabbitMQ for object recovery"""
import traceback
import time
import json
import pika

from s3backgrounddelete.cortx_s3_kv_api import CORTXS3KVApi
from s3backgrounddelete.cortx_s3_object_api import CORTXS3ObjectApi
from s3backgrounddelete.cortx_s3_index_api import CORTXS3IndexApi
from s3backgrounddelete.object_recovery_validator import ObjectRecoveryValidator
from s3backgrounddelete.IEMutil import IEMutil


class ObjectRecoveryRabbitMq(object):
    """This class is implementation of RabbitMq for object recovery."""
    _connection = None
    _channel = None

    def __init__(self, config, user, password, host,
                 exchange, queue, mode, durable, logger):
        """Initialise rabbitmq"""
        self.config = config
        self._user = user
        self._password = password
        self._host = host
        self._exchange = exchange
        self._mode = mode
        self._queue = queue
        self._durable = True if durable == "True" else False
        self.logger = logger
        self.connect()

    def connect(self):
        """Connect to message queue"""
        try:
            credentials = pika.PlainCredentials(self._user, self._password)
            self._connection = pika.BlockingConnection(
                pika.ConnectionParameters(host=self._host, credentials=credentials))
            self._channel = self._connection.channel()
            self._channel.queue_declare(
                queue=self._queue, durable=self._durable)
        except Exception as exception:
            err_msg = "error:%s, %s" % (exception, traceback.format_exc())
            self.logger.warn("msg_queue connect failed." + str(err_msg))
            time.sleep(5)
            self.connect()

    def send_data(self, data, mq_routing):
        """Send message data."""
        try:
            self._channel.basic_publish(exchange=self._exchange,
                                        routing_key=mq_routing,
                                        body=json.dumps(data),
                                        properties=pika.BasicProperties(
                                            delivery_mode=self._mode,  # make message persistent
                                        ))
        except Exception as exception:
            msg = ("msg_queue send data except:%s, %s") % (
                exception, traceback.format_exc())
            return False, msg

        return True, None

    def worker(self, queue_msg_count=None):
        """Create worker which will process results."""
        # Callback function to consume the queue messages and parameters are,
        # channel : rabbitmq Channel used to send/receive/ack messages
        # method :  contain details to identify which consumer the message should
        #           go e.g delivery_tag
        # properties : BasicProperties contains message properties (metadata)
        # body : message body
        # example:
        # method: <Basic.GetOk(['delivery_tag=1', 'exchange=',
        #         'message_count=0', 'redelivered=False',
        #         'routing_key=s3_delete_obj_job_queue'])>
        # properties: <BasicProperties(['delivery_mode=2'])>
        # body: b'{"Key": "egZPBQAAAAA=-ZQIAAAAAJKc=",
        #       "Value": "{\\"index_id\\":\\"egZPBQAAAHg=-YwIAAAAAJKc=\\",
        #       \\"object_layout_id\\":1,
        #        \\"object_metadata_path\\":\\"object1\\"}\\n"}'
        def callback(channel, method, properties, body):
            """Process the result and send acknowledge."""
            try:
                self.logger.info(
                    "Received " +
                    body.decode("utf-8") +
                    "at consumer end")
                probable_delete_records = json.loads(body.decode("utf-8"))
                if (probable_delete_records is not None):
                    self.logger.info(
                        "Processing following records in consumer " +
                        str(probable_delete_records))
                    validator = ObjectRecoveryValidator(
                        self.config, probable_delete_records, self.logger)
                    validator.process_results()
                channel.basic_ack(delivery_tag=method.delivery_tag)

            except BaseException:
                self.logger.error(
                    "msg_queue callback failed." + traceback.format_exc())

        self._channel.basic_qos(prefetch_count=1)

        # If service is running in non-daemon mode,
        # then consume messages till the queue is empty and then stop
        # else start consuming the message till service stops.
        if (queue_msg_count is not None):
            self.logger.info("Queue contains " +  str(queue_msg_count) + " messages")
            for msg in range(queue_msg_count, 0, -1):
                method, properties, body = self._channel.basic_get(self._queue, no_ack=False)
                callback(self._channel, method, properties, body)
            self.logger.info("Consumed all messages and queue is empty")
            return
        else:
            self._channel.basic_consume(callback, self._queue, no_ack=False)
            self._channel.start_consuming()


    def receive_data(self):
        """Receive data and create msg queue."""

        try:
            # Check if service is running in non-daemon mode
            # then consumer should stop once queue is empty.
            if not self.config.get_daemon_mode():
                queue_state = self._channel.queue_declare(
                    queue=self._queue, durable=self._durable)
                queue_msg_count = queue_state.method.message_count
                self.worker(queue_msg_count)
                return
            else:
                self._channel.queue_declare(
                    queue=self._queue, durable=self._durable)
                self.worker()

        except Exception as exception:
            err_msg = "error:%s, %s" % (exception, traceback.format_exc())
            IEMutil("ERROR", IEMutil.RABBIT_MQ_CONN_FAILURE, IEMutil.RABBIT_MQ_CONN_FAILURE_STR)
            self.logger.error("msg_queue receive data failed." + str(err_msg))
            self.connect()
            self.receive_data()

    def close(self):
        """Stop consumer and close rabbitmq connection."""
        try:
            self._channel.stop_consuming()
        finally:
            self._connection.close()
