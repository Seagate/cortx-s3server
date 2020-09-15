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

"""
This class act as object recovery processor which consumes
the rabbitmq message queue.
"""
#!/usr/bin/python3.6

import os
import traceback
import logging
import datetime
from logging import handlers

from s3backgrounddelete.object_recovery_queue import ObjectRecoveryRabbitMq
from s3backgrounddelete.cortx_s3_config import CORTXS3Config


class ObjectRecoveryProcessor(object):
    """Provides consumer for object recovery"""

    def __init__(self):
        """Initialise Server, config and create logger."""
        self.server = None
        self.config = CORTXS3Config()
        self.create_logger_directory()
        self.create_logger()
        self.logger.info("Initialising the Object Recovery Processor")

    def consume(self):
        """Consume the objects from object recovery queue."""
        self.server = None
        try:
             self.server = ObjectRecoveryRabbitMq(
                 self.config,
                 self.config.get_rabbitmq_username(),
                 self.config.get_rabbitmq_password(),
                 self.config.get_rabbitmq_host(),
                 self.config.get_rabbitmq_exchange(),
                 self.config.get_rabbitmq_queue_name(),
                 self.config.get_rabbitmq_mode(),
                 self.config.get_rabbitmq_durable(),
                 self.logger)
             self.logger.info("Consumer started at " +
                             str(datetime.datetime.now()))
             self.server.receive_data()
        except BaseException:
            if self.server:
                self.server.close()
            self.logger.error("main except:" + str(traceback.format_exc()))

    def create_logger(self):
        """Create logger, file handler, formatter."""
        # Create logger with "object_recovery_processor"
        self.logger = logging.getLogger(
            self.config.get_processor_logger_name())
        self.logger.setLevel(self.config.get_file_log_level())
        # create file handler which logs even debug messages
        fhandler = logging.handlers.RotatingFileHandler(self.config.get_processor_logger_file(), mode='a',
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

    def close(self):
        """Stop processor and close rabbitmq connection."""
        self.logger.info("Stopping the processor and rabbitmq connection")
        self.server.close()
        # perform an orderly shutdown by flushing and closing all handlers
        logging.shutdown()

    def create_logger_directory(self):
        """Create log directory if not exsists."""
        self._logger_directory = os.path.join(self.config.get_logger_directory())
        if not os.path.isdir(self._logger_directory):
            try:
                os.mkdir(self._logger_directory)
            except BaseException:
                self.logger.error(
                    "Unable to create log directory at " + self._logger_directory)

if __name__ == "__main__":
    PROCESSOR = ObjectRecoveryProcessor()
    PROCESSOR.consume()
