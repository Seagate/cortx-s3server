"""
This class act as object recovery processor which consumes
the rabbitmq mesaage queue.
"""
#!/usr/bin/python3

import traceback
import logging
import datetime

import object_recovery_queue
from eos_core_config import EOSCoreConfig


class ObjectRecoveryProcessor(object):
    """Provides consumer for object recovery"""

    def __init__(self):
        """Initialise Server, config and create logger."""
        self.server = None
        self.config = EOSCoreConfig()
        self.create_logger()

    def consume(self):
        """Consume the objects from object recovery queue."""
        self.server = None
        try:
            self.server = object_recovery_queue.ObjectRecoveryRabbitMq(
                self.config,
                self.config.get_rabbitmq_username(),
                self.config.get_rabbitmq_password(),
                self.config.get_rabbitmq_host(),
                self.config.get_rabbitmq_exchange(),
                self.config.get_rabbitmq_queue_name(),
                self.config.get_rabbitmq_mode(),
                self.config.get_rabbitmq_durable(),
                self.logger)
            self.logger.info(" Consumer started at" +
                             str(datetime.datetime.now()))
            self.server.receive_data()
        except BaseException:
            if self.server:
                self.server.close()
            self.logger.error(" main except:" + str(traceback.format_exc()))

    def create_logger(self):
        """Create logger, file handler, formatter."""
        # Create logger with "object_recovery_processor"
        self.logger = logging.getLogger(
            self.config.get_processor_logger_name())
        self.logger.setLevel(self.config.get_file_log_level())
        # create file handler which logs even debug messages
        fhandler = logging.FileHandler(self.config.get_processor_logger_file())
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
    PROCESSOR = ObjectRecoveryProcessor()
    PROCESSOR.consume()
