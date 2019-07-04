#!/usr/bin/python3

import config
import object_recovery_queue
import traceback
import sched
import time
import logging
import datetime
import json

from eos_core_config import EOSCoreConfig
from eos_core_kv_api import EOSCoreKVApi
from eos_core_object_api import EOSCoreObjectApi
from eos_core_index_api import EOSCoreIndexApi
from eos_list_index_response import EOSCoreListIndexResponse

class ObjectRecoveryScheduler:

    def __init__(self):
        self.data = None
        self.config =  EOSCoreConfig()
        self.create_logger()

    def add_kv_to_queue(self):
        try:
            mq_client = object_recovery_queue.ObjectRecoveryRabbitMq(self.config,
                                                     self.config.get_rabbitmq_username(),
                                                     self.config.get_rabbitmq_password(),
                                                     self.config.get_rabbitmq_host(),
                                                     self.config.get_rabbitmq_exchange(),
                                                     self.config.get_rabbitmq_queue_name(),
                                                     self.config.get_rabbitmq_mode(),
                                                     self.config.get_rabbitmq_durable(),
                                                     self.logger)
            result, index_response = EOSCoreIndexApi(self.config, self.logger).list(self.config.get_probable_delete_index_id())
            if (result) :
                self.logger.info(" Index listing result :" + str(index_response.get_index_content()))
                probable_delete_oid = index_response.get_index_content()
                self.logger.info(" Object recovery queue sending data :" + str(probable_delete_oid))
                ret, msg = mq_client.send_data(probable_delete_oid, self.config.get_rabbitmq_queue_name())
                if not ret:
                    self.logger.error(" Object recovery queue send data failed :" + str(probable_delete_oid))
                    self.logger.info(" Object recovery queue send data successfully :" + str(probable_delete_oid))
        except:
            self.logger.error(
                " Object recovery queue send data exception:" + traceback.format_exc())
        finally:
            if mq_client:
                mq_client.close()

    def schedule_periodically(self):
         # Run RabbitMQ producer periodically on hourly basis
        self.logger.info(" Producer started at" + str(datetime.datetime.now()))
        scheduled_run = sched.scheduler(time.time, time.sleep)

        def periodic_run(sc):
            self.add_kv_to_queue()
            scheduled_run.enter(self.config.get_schedule_interval(), 1, periodic_run, (sc,))

        scheduled_run.enter(self.config.get_schedule_interval(), 1, periodic_run, (scheduled_run,))
        scheduled_run.run()

    def create_logger(self):
        # create logger with "object_recovery_scheduler"
        self.logger = logging.getLogger(self.config.get_scheduler_logger_name())
        self.logger.setLevel(self.config.get_file_log_level())
        # create file handler which logs even debug messages
        # Todo : Use a RotatingFileHandler https://docs.python.org/3/library/logging.handlers.html#logging.handlers.RotatingFileHandler
        fh = logging.FileHandler(self.config.get_scheduler_logger_file())
        fh.setLevel(self.config.get_file_log_level())
        # create console handler with a higher log level
        ch = logging.StreamHandler()
        ch.setLevel(self.config.get_console_log_level())
        # create formatter and add it to the handlers
        formatter = logging.Formatter(self.config.get_log_format())
        fh.setFormatter(formatter)
        ch.setFormatter(formatter)
        # add the handlers to the logger
        self.logger.addHandler(fh)
        self.logger.addHandler(ch)

if __name__ == "__main__":
    scheduler = ObjectRecoveryScheduler()
    scheduler.schedule_periodically()
