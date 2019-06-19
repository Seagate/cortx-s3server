#!/usr/bin/python3

import time
import object_recovery_queue
import traceback
import logging
import datetime
from config import RabbitMQConfig

class ObjectRecoveryProcessor:

    def __init__(self):
        self.server = None
        self.create_logger()

    def consume(self):
        self.server = None
        try:
            self.server = object_recovery_queue.ObjectRecoveryRabbitMq(RabbitMQConfig.user,
                                                       RabbitMQConfig.password,
                                                       RabbitMQConfig.host,
                                                       RabbitMQConfig.exchange,
                                                       RabbitMQConfig.queque,
                                                       RabbitMQConfig.mode,
                                                       RabbitMQConfig.durable,
                                                       self.logger)
            self.logger.info(" Consumer started at" +
                             str(datetime.datetime.now()))
            self.server.receive_data()
        except:
            if self.server:
                self.server.close()
            self.logger.error(" main except:" + str(traceback.format_exc()))

    def create_logger(self):
        # create logger with "object_recovery_processor"
        self.logger = logging.getLogger("object_recovery_processor")
        self.logger.setLevel(logging.DEBUG)
        # create file handler which logs even debug messages
        fh = logging.FileHandler("object_recovery_processor.log")
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
    processor = ObjectRecoveryProcessor()
    processor.consume()
