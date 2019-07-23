"""Implementation of RabbitMQ for object recovery"""
import traceback
import time
import json
import pika

from eos_core_kv_api import EOSCoreKVApi
from eos_core_object_api import EOSCoreObjectApi
from eos_core_index_api import EOSCoreIndexApi
from object_recovery_validator import ObjectRecoveryValidator


class ObjectRecoveryRabbitMq(object):
    """This class is implementation of RabbitMq for object recovery."""
    _connection = None
    _channel = None

    def __init__(self, config, user, password, host,
                 exchange, queque, mode, durable, logger):
        """Initialise rabbitmq"""
        self.config = config
        self._user = user
        self._password = password
        self._host = host
        self._exchange = exchange
        self._mode = mode
        self._queque = queque
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
                queue=self._queque, durable=self._durable)
        except Exception as exception:
            err_msg = "error:%s, %s" % (exception, traceback.format_exc())
            self.logger.warn("msg_queque connect failed." + str(err_msg))
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
            msg = ("msg_queque send data except:%s, %s") % (
                exception, traceback.format_exc())
            return False, msg

        return True, None

    def worker(self):
        """Create worker which will process results."""
        def callback(channel, method, properties, body):
            """Process the result and send acknowledge."""
            try:
                self.logger.info(
                    " Received " +
                    body.decode("utf-8") +
                    "at consumer end")
                probable_delete_records = json.loads(body.decode("utf-8"))
                if (probable_delete_records is not None):
                    self.logger.info(
                        " Processing following records in consumer " +
                        str(probable_delete_records))
                    validator = ObjectRecoveryValidator(
                        self.config, probable_delete_records, self.logger)
                    validator.process_results()
                channel.basic_ack(delivery_tag=method.delivery_tag)
            except BaseException:
                self.logger.error(
                    "msg_queque callback failed." + traceback.format_exc())

        self._channel.basic_qos(prefetch_count=1)
        self._channel.basic_consume(callback, self._queque, no_ack=False)
        self._channel.start_consuming()

    def receive_data(self):
        """Receive data and create msg queue."""
        try:
            self._channel.queue_declare(
                queue=self._queque, durable=self._durable)
            self.worker()

        except Exception as exception:
            err_msg = "error:%s, %s" % (exception, traceback.format_exc())
            self.logger.error("msg_queque receive data failed." + str(err_msg))
            self.connect()
            self.receive_data()

    def close(self):
        """Stop consumer and close rabbitmq connection."""
        try:
            self._channel.stop_consuming()
        finally:
            self._connection.close()
