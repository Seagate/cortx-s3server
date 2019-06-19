#!/usr/bin/python3

import config
import object_recovery_queue
import traceback
import sched
import time
import logging
import datetime
import json

from config import RabbitMQConfig
from eos_core_kv_api import EOSCoreKVApi
from eos_core_object_api import EOSCoreObjectApi
from eos_core_index_api import EOSCoreIndexApi
from eos_index_response import EOSCoreIndexResponse

class ObjectRecoveryScheduler:

    def __init__(self):
        self.data = None
        self.create_logger()

    def add_kv_to_queue(self):
        try:
            mq_client = object_recovery_queue.ObjectRecoveryRabbitMq(RabbitMQConfig.user,
                                                     RabbitMQConfig.password,
                                                     RabbitMQConfig.host,
                                                     RabbitMQConfig.exchange,
                                                     RabbitMQConfig.queque,
                                                     RabbitMQConfig.mode,
                                                     RabbitMQConfig.durable,
                                                     self.logger)
            index_response = EOSCoreIndexApi().list("probable_delete_index-id")
            if (index_response.get_status() == 200 ):
                self.logger.info(" Index listing result :" + str(index_response.get_index_content()))
                probable_delete_oid = index_response.get_index_content()
                for oid in probable_delete_oid:
                    self.logger.info(" msg_queque sending data :" + str(oid))
                    ret, msg = mq_client.send_data(oid, RabbitMQConfig.queque)
                    if not ret:
                        self.logger.error(" msg_queque send data failed :" + msg)
                        self.logger.info(" msg_queque send data successfully :" + oid)
        except:
            self.logger.error(
                " msg_queque send data except:" + traceback.format_exc())
        finally:
            if mq_client:
                mq_client.close()

    def schedule_periodically(self):
         # Run RabbitMQ producer periodically on hourly basis
        self.logger.info(" Producer started at" + str(datetime.datetime.now()))
        scheduled_run = sched.scheduler(time.time, time.sleep)

        def periodic_run(sc):
            self.add_kv_to_queue()
            scheduled_run.enter(RabbitMQConfig.schedule_interval, 1, periodic_run, (sc,))

        scheduled_run.enter(RabbitMQConfig.schedule_interval, 1, periodic_run, (scheduled_run,))
        scheduled_run.run()

    def create_logger(self):
        # create logger with "object_recovery_scheduler"
        self.logger = logging.getLogger("object_recovery_scheduler")
        self.logger.setLevel(logging.DEBUG)
        # create file handler which logs even debug messages
        # Todo : Use a RotatingFileHandler https://docs.python.org/3/library/logging.handlers.html#logging.handlers.RotatingFileHandler
        fh = logging.FileHandler("object_recovery_scheduler.log")
        fh.setLevel(logging.DEBUG)
        # create console handler with a higher log level
        ch = logging.StreamHandler()
        ch.setLevel(logging.ERROR)
        # create formatter and add it to the handlers
        formatter = logging.Formatter(
            "%(asctime)s - %(name)s - %(levelname)s - %(message)s")
        fh.setFormatter(formatter)
        ch.setFormatter(formatter)
        # add the handlers to the logger
        self.logger.addHandler(fh)
        self.logger.addHandler(ch)

if __name__ == "__main__":
    scheduler = ObjectRecoveryScheduler()
    scheduler.schedule_periodically()
