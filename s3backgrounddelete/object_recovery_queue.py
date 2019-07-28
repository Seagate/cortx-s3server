"""Implementation of RabbitMQ for object recovery"""
import traceback
import time
import json
import pika

from eos_core_kv_api import EOSCoreKVApi
from eos_core_object_api import EOSCoreObjectApi


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

    def perform_validations(self, probable_delete_oid, object_metadata_oid):
        """Validate delete object oid with object metadata oid."""
        if (probable_delete_oid == object_metadata_oid):
            return True
        self.logger.info("Oid mismatch in object metadata")
        return False

    def process_results(self, probable_delete_index):
        """Delete probable object."""
        for key, value in probable_delete_index.items():
            probable_delete_object_name = json.loads(value)["obj-name"]
            self.logger.info(
                "Probable object id to be deleted : " + probable_delete_object_name)
            result, get_response = EOSCoreKVApi(self.config, self.logger).get(
                self.config.get_object_metadata_index_id(), key)
            if(result):
                object_metadata = json.loads(get_response.get_value())
                object_metadata_oid = object_metadata["mero_oid_u_hi"]
                if(self.perform_validations(key, object_metadata_oid)):
                    self.logger.info(
                        "Deleting following object Id : " + str(key))
                    EOSCoreObjectApi(self.config, self.logger).delete(key)
                    EOSCoreKVApi(self.config, self.logger).delete(
                        self.config.get_probable_delete_index_id(), key)
                else:
                    EOSCoreKVApi(self.config, self.logger).delete(
                        self.config.get_probable_delete_index_id(), key)
                    self.logger.info(
                        "Object name mismatch entry in metadata found for : "
                        + probable_delete_object_name)
            else:
                self.logger.error(
                    "Failed to get object metadata for : " + probable_delete_object_name)

    def worker(self):
        """Create worker which will process results."""
        def callback(channel, method, properties, body):
            """Process the result and send acknowledge."""
            try:
                if (body is not None):
                    self.logger.info(
                        " Received " + body.decode("utf-8") + "at consumer end")
                    self.logger.info(
                        " Processing following records in consumer" + body.decode("utf-8"))
                    self.process_results(json.loads(body.decode("utf-8")))
                channel.basic_ack(delivery_tag=method.delivery_tag)
            except BaseException:
                self.logger.error(
                    "msg_queque callback failed." + traceback.format_exc())

        self._channel.basic_qos(prefetch_count=1)
        self._channel.basic_consume(self._queque, callback, auto_ack=False)
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
