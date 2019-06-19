import pika
import sys
import traceback
import time
import os
from datetime import datetime
from time import gmtime, strftime
import json

from eos_kv_response import EOSCoreKVResponse
from config import RabbitMQConfig
from eos_core_kv_api import EOSCoreKVApi
from eos_core_object_api import EOSCoreObjectApi
from eos_core_index_api import EOSCoreIndexApi

class ObjectRecoveryRabbitMq:
    _connection = None
    _channel = None

    def __init__(self, user, password, host, exchange, queque, mode, durable, logger):
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
        try:
            credentials = pika.PlainCredentials(self._user, self._password)
            self._connection = pika.BlockingConnection(
                pika.ConnectionParameters(host=self._host, credentials=credentials))
            self._channel = self._connection.channel()
            self._channel.queue_declare(queue=self._queque, durable=self._durable)
        except Exception as e:
            err_msg = "error:%s, %s" % (e, traceback.format_exc())
            self.logger.warn("msg_queque connect failed." + str(err_msg))
            time.sleep(5)
            self.connect()

    def send_data(self, data, mq_routing):
        try:
            self._channel.basic_publish(exchange=self._exchange,
                                       routing_key=mq_routing,
                                       body=data,
                                       properties=pika.BasicProperties(
                                           delivery_mode=self._mode,  # make message persistent
                                       ))
        except Exception as e:
            msg = ("msg_queque send data except:%s, %s") % (
                e, traceback.format_exc())
            return False, msg

        return True, None

    def perform_validations(self, object_metadata, probable_object_name):
        if( object_metadata != str("Object not found.") ):
            self.logger.info("Object exists " + str(probable_object_name))
            if ( object_metadata["Object-Name"]== probable_object_name ):
                return True
        self.logger.info("Object doesn't exists " + str(probable_object_name))
        return False

    def process_results(self, oid):

        kv_response = EOSCoreKVApi().get("probable_delete_index-id", oid)
        if ( kv_response.get_status() == 200 ) :
            probable_object_name = kv_response.get_json_parsed_value()
        self.logger.info("Probable object name to be deleted : " + str(probable_object_name))
        object_metadata = EOSCoreKVApi().get("object-metadata-index-id", oid).get_value()
        if( self.perform_validations(json.loads(object_metadata), probable_object_name) ):
            self.logger.info("Deleting following object Id : " + str(oid))
            EOSCoreObjectApi().delete(oid)

    def worker(self):
        def callback(channel, method, properties, body):
            try:
                self.logger.info(" Received " + body.decode("utf-8") + "at consumer end")
                self.logger.info(" Processing following records in consumer"+body.decode("utf-8"))
                self.process_results(body.decode("utf-8"))
                channel.basic_ack(delivery_tag=method.delivery_tag)

            except:
                self.logger.error(
                    "msg_queque callback failed." + traceback.format_exc())

        self._channel.basic_qos(prefetch_count=1)
        self._channel.basic_consume(callback, self._queque, no_ack=False)
        self._channel.start_consuming()

    def receive_data(self):
        try:
            self._channel.queue_declare(queue=self._queque, durable=self._durable)
            self.worker()

        except Exception as e:
            err_msg = "error:%s, %s" % (e, traceback.format_exc())
            self.logger.error("msg_queque receive data failed." + str(err_msg))
            self.connect()
            self.receive_data()

    def close(self):
        try:
            self._channel.stop_consuming()
        finally:
            self._connection.close()
