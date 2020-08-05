"""Implementation of RabbitMQ for object recovery"""
import traceback
import time
import json
import pika
import os
import datetime
import math

from s3backgrounddelete.eos_core_config import EOSCoreConfig
from s3backgrounddelete.eos_core_kv_api import EOSCoreKVApi
from s3backgrounddelete.eos_core_object_api import EOSCoreObjectApi
from s3backgrounddelete.eos_core_index_api import EOSCoreIndexApi
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

    def is_object_older_than_movedays(self, last_modified_date, move_after_days):
        if move_after_days == -1:
            return True
        now = datetime.datetime.utcnow()
        self.logger.info("last modified date of object: " + last_modified_date)
        self.logger.info("Now time: " + str(now))

        date_time_obj = datetime.datetime.strptime(last_modified_date, "%Y-%m-%dT%H:%M:%S.000Z")
        time_delta = now - date_time_obj
        time_delta_in_days = math.floor(time_delta.total_seconds()/86400)

        return (time_delta_in_days >= int(move_after_days))

    def download_objects_from_seagate(self, source_bucket, days):
        cmd = "aws s3api --endpoint http://s3.seagate.com --profile seagate list-objects --bucket "
        cmd += source_bucket
        downloaded_object_list = list()
        stream = os.popen(cmd)
        output = stream.read()
        if output != '':
            output_json = json.loads(output)
            objects_list = output_json["Contents"]
            for object in objects_list:
                key = object["Key"]
                size = object["Size"]
                last_modified = object["LastModified"]
                if self.is_object_older_than_movedays(last_modified, days) == True:
                    cmd = "aws s3api --endpoint http://s3.seagate.com --profile seagate get-object --bucket "
                    cmd += source_bucket
                    cmd += " --key "
                    cmd += key
                    cmd += " "
                    cmd += key
                    stream = os.popen(cmd)
                    output = stream.read()
                    output_json = json.loads(output)
                    if output_json["ContentLength"] == size:
                        self.logger.info("Successfully downloaded object: " + key)
                        downloaded_object_list.append(key)
                    else:
                        self.logger.info("Failed to downloaded object: " + key)
                else:
                    self.logger.info("Key: " + key + "'s modified date is not older than: " + str(days))
        return downloaded_object_list

    def upload_objects_to_cloud(self, objects_list, bucket, access_key, secret_key):
        # list and download source_bucket objets
        os.environ["AWS_ACCESS_KEY_ID"] = access_key
        os.environ["AWS_SECRET_ACCESS_KEY"] = secret_key

        basecmd = "aws s3api --endpoint http://s3.amazonaws.com put-object --bucket "
        basecmd += bucket
        basecmd += " --key "
        for object in objects_list:
            cmd = basecmd + object
            cmd += " --body "
            cmd += object
            self.logger.info("Executing upload object cmd: " + cmd)
            os.system(cmd)
            cmd = ''

    def get_public_cloud_info(self, bucket):
        cmd = "aws s3api --endpoint http://s3.seagate.com --profile seagate get-bucket-tagging --bucket "
        cmd += bucket
        stream = os.popen(cmd)
        output = stream.read()
        if output != '':
            move_after_days = 60 # default value
            expire_after_copy = "True" # default value
            output_json = json.loads(output)
            tags_list = output_json["TagSet"]
            for tag in tags_list:
                if tag["Key"] == "Access-Key":
                    access_key = tag["Value"]
                elif tag["Key"] == "DestBucket":
                    upload_bucket = tag["Value"]
                elif tag["Key"] == "Secret-Key":
                    secret_key = tag["Value"]
                elif tag["Key"] == "MoveAfterDays":
                    move_after_days = tag["Value"]
                elif tag["Key"] == "ExpireAfterCopy":
                    expire_after_copy = tag["Value"]

        return access_key, secret_key, upload_bucket, move_after_days, expire_after_copy

    def delete_objects(self, bucket_name, objects_list):
        deletecmd = "aws s3api --endpoint http://s3.seagate.com --profile seagate delete-object --bucket "
        deletecmd += bucket_name
        deletecmd += " --key "
        for object in objects_list:
            self.logger.info("Executing delete-object cmd: " + (deletecmd + object))
            os.system(deletecmd + object)
            #output = stream.read()
            #output_json = json.loads(output)
            #if output_json["DeleteMarker"] == True:

    def replication_worker(self, queue_msg_count=None):
        def callback(channel, method, properties, body):
            """Process the result and send acknowledge."""
            try:
                self.logger.info(
                    "Received " + body.decode("utf-8") + "at consumer end"
                )
                replication_record = json.loads(body.decode("utf-8"))
                if (replication_record is not None):
                    self.logger.info(
                        "Processing following records in consumer " +
                        str(replication_record))
                    # process the record
                    seagate_bucket = replication_record
                    access_key, secret_key, upload_bucket, move_after_days, expire_after_copy = self\
                        .get_public_cloud_info(seagate_bucket)

                    # download the objects of the bucket
                    upload_objects_list = self.download_objects_from_seagate(seagate_bucket, move_after_days)

                    # upload the downloaded objects to public cloud
                    if len(upload_objects_list) > 0:
                        self.upload_objects_to_cloud(upload_objects_list, upload_bucket, access_key, secret_key)
                        if expire_after_copy == "True":
                            # delete the objects
                            self.delete_objects(seagate_bucket, upload_objects_list)
                    else:
                        self.logger.info("No object of bucket: " + seagate_bucket + " is eligible for upload.")

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
            if (self.config.get_daemon_mode() == "False"):
                queue_state = self._channel.queue_declare(
                    queue=self._queue, durable=self._durable)
                queue_msg_count = queue_state.method.message_count
                #self.worker(queue_msg_count)
                self.replication_worker(queue_msg_count)
                return
            else:
                self._channel.queue_declare(
                    queue=self._queue, durable=self._durable)
                self.replication_worker(queue_msg_count)

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
