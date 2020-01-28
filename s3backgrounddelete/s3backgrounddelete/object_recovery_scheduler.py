"""
This class act as object recovery scheduler which add key value to
the rabbitmq mesaage queue.
"""
#!/usr/bin/python3.6

import traceback
import sched
import time
import logging
from logging import handlers
import datetime

from s3backgrounddelete.object_recovery_queue import ObjectRecoveryRabbitMq
from s3backgrounddelete.eos_core_config import EOSCoreConfig
from s3backgrounddelete.eos_core_index_api import EOSCoreIndexApi


class ObjectRecoveryScheduler(object):
    """Scheduler which will add key value to rabbitmq message queue."""

    def __init__(self):
        """Initialise logger and configuration."""
        self.data = None
        self.config = EOSCoreConfig()
        self.create_logger()
        self.logger.info("Initialising the Object Recovery Scheduler")

    def add_kv_to_queue(self, marker = None):
        """Add object key value to object recovery queue."""
        self.logger.info("Adding kv list to queue")
        try:
            mq_client = ObjectRecoveryRabbitMq(
                self.config,
                self.config.get_rabbitmq_username(),
                self.config.get_rabbitmq_password(),
                self.config.get_rabbitmq_host(),
                self.config.get_rabbitmq_exchange(),
                self.config.get_rabbitmq_queue_name(),
                self.config.get_rabbitmq_mode(),
                self.config.get_rabbitmq_durable(),
                self.logger)
            result, index_response = EOSCoreIndexApi(
                self.config, logger=self.logger).list(
                    self.config.get_probable_delete_index_id(), self.config.get_max_keys(), marker)
            if result:
                self.logger.info("Index listing result :" +
                                 str(index_response.get_index_content()))
                probable_delete_json = index_response.get_index_content()
                probable_delete_oid_list = probable_delete_json["Keys"]
                is_truncated = probable_delete_json["IsTruncated"]
                if (probable_delete_oid_list is not None):
                    for record in probable_delete_oid_list:
                        self.logger.info(
                            "Object recovery queue sending data :" +
                            str(record))
                        ret, msg = mq_client.send_data(
                            record, self.config.get_rabbitmq_queue_name())
                        if not ret:
                            self.logger.error(
                                "Object recovery queue send data "+ str(record) +
                                " failed :" + msg)
                        else:
                            self.logger.info(
                                "Object recovery queue send data successfully :" +
                                str(record))
                    if (is_truncated == "true"):
                        self.add_kv_to_queue(probable_delete_json["NextMarker"])
                else:
                    self.logger.info(
                        "Index listing result empty. Ignoring adding entry to object recovery queue")
                    pass
        except BaseException:
            self.logger.error(
                "Object recovery queue send data exception:" + traceback.format_exc())
        finally:
            if mq_client:
               self.logger.info("Closing the mqclient")
               mq_client.close()

    def schedule_periodically(self):
        """Schedule RabbitMQ producer to add key value to queue on hourly basis."""
        # Run RabbitMQ producer periodically on hourly basis
        self.logger.info("Producer started at " + str(datetime.datetime.now()))
        scheduled_run = sched.scheduler(time.time, time.sleep)

        def periodic_run(scheduler):
            """Add key value to queue using scheduler."""
            self.add_kv_to_queue()
            scheduled_run.enter(
                self.config.get_schedule_interval(), 1, periodic_run, (scheduler,))

        scheduled_run.enter(self.config.get_schedule_interval(),
                            1, periodic_run, (scheduled_run,))
        scheduled_run.run()

    def create_logger(self):
        """Create logger, file handler, console handler and formatter."""
        # create logger with "object_recovery_scheduler"
        self.logger = logging.getLogger(
            self.config.get_scheduler_logger_name())
        self.logger.setLevel(self.config.get_file_log_level())
        # https://docs.python.org/3/library/logging.handlers.html#logging.handlers.RotatingFileHandler
        fhandler = logging.handlers.RotatingFileHandler(self.config.get_scheduler_logger_file(), mode='a',
                                                        maxBytes = self.config.get_max_bytes(),
                                                        backupCount = self.config.get_backup_count(), encoding=None,
                                                        delay=False )
        fhandler.setLevel(self.config.get_file_log_level())
        # create console handler with a higher log level
        chandler = logging.StreamHandler()
        chandler.setLevel(self.config.get_console_log_level())
        # create formatter and add it to the handlers
        formatter = logging.Formatter(self.config.get_log_format())
        fhandler.setFormatter(formatter)
        chandler.setFormatter(formatter)
        # add the handlers to the logger
        self.logger.addHandler(fhandler)
        self.logger.addHandler(chandler)

if __name__ == "__main__":
    SCHEDULER = ObjectRecoveryScheduler()
    SCHEDULER.schedule_periodically()
