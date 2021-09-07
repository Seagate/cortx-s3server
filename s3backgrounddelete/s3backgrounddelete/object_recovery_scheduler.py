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
This class act as object recovery scheduler which add key value to
the underlying messaging platform.
"""
#!/usr/bin/python3.6

import os
import traceback
import sched
import time
import logging
from logging import handlers
import datetime
import math
import json
import signal
import sys

from s3backgrounddelete.cortx_s3_config import CORTXS3Config
from s3backgrounddelete.cortx_s3_index_api import CORTXS3IndexApi
from s3backgrounddelete.cortx_s3_signal import DynamicConfigHandler
from s3backgrounddelete.cortx_s3_constants import MESSAGE_BUS
from s3backgrounddelete.cortx_s3_constants import CONNECTION_TYPE_PRODUCER
#from s3backgrounddelete.IEMutil import IEMutil

class ObjectRecoveryScheduler(object):
    """Scheduler which will add key value to message_bus queue."""

    def __init__(self, producer_name):
        """Initialise logger and configuration."""
        self.data = None
        self.config = CORTXS3Config()
        self.create_logger_directory()
        self.create_logger()
        self.signal = DynamicConfigHandler(self)
        self.logger.info("Initialising the Object Recovery Scheduler")
        self.producer = None
        self.producer_name = producer_name

    @staticmethod
    def isObjectLeakEntryOlderThan(leakRecord, OlderInMins = 15):
        object_leak_time = leakRecord["create_timestamp"]
        now = datetime.datetime.utcnow()
        date_time_obj = datetime.datetime.strptime(object_leak_time, "%Y-%m-%dT%H:%M:%S.000Z")
        timeDelta = now - date_time_obj
        timeDeltaInMns = math.floor(timeDelta.total_seconds()/60)
        return (timeDeltaInMns >= OlderInMins)

    def add_kv_to_msgbus(self, marker = None):
        """Add object key value to msgbus topic."""
        self.logger.info("Inside add_kv_to_msgbus.")
        try:
            from s3backgrounddelete.object_recovery_msgbus import ObjectRecoveryMsgbus

            if not self.producer:
                self.producer = ObjectRecoveryMsgbus(
                    self.config,
                    self.logger)
            threshold = self.config.get_threshold()
            self.logger.debug("Threshold is : " + str(threshold))
            count = self.producer.get_count()
            self.logger.debug("Count of unread msgs is : " + str(count))

            if ((int(count) < threshold) or (threshold == 0)):
                self.logger.debug("Count of unread messages is less than threshold value.Hence continuing...")
            else:
                #do nothing
                self.logger.info("Queue has more messages than threshold value. Hence skipping addition of further entries.")
                return
            # Cleanup all entries and enqueue only 1000 entries
            #PurgeAPI Here
            self.producer.purge()
            result, index_response = CORTXS3IndexApi(
                self.config, connectionType=CONNECTION_TYPE_PRODUCER, logger=self.logger).list(
                    self.config.get_probable_delete_index_id(), self.config.get_max_keys(), marker)
            if result:
                self.logger.info("Index listing result :" +
                                 str(index_response.get_index_content()))
                probable_delete_json = index_response.get_index_content()
                probable_delete_oid_list = probable_delete_json["Keys"]
                is_truncated = probable_delete_json["IsTruncated"]
                if (probable_delete_oid_list is not None):
                    for record in probable_delete_oid_list:
                        # Check if record is older than the pre-configured 'time to process' delay
                        leak_processing_delay = self.config.get_leak_processing_delay_in_mins()
                        try:
                            objLeakVal = json.loads(record["Value"])
                        except ValueError as error:
                            self.logger.error(
                            "Failed to parse JSON data for: " + str(record) + " due to: " + error)
                            continue

                        if (objLeakVal is None):
                            self.logger.error("No value associated with " + str(record) + ". Skipping entry")
                            continue

                        # Check if object leak entry is older than 15mins or a preconfigured duration
                        if (not ObjectRecoveryScheduler.isObjectLeakEntryOlderThan(objLeakVal, leak_processing_delay)):
                            self.logger.info("Object leak entry " + record["Key"] +
                                            " is NOT older than " + str(leak_processing_delay) +
                                            "mins. Skipping entry")
                            continue

                        self.logger.info(
                            "Object recovery queue sending data :" +
                            str(record))
                        ret = self.producer.send_data(record, producer_id = self.producer_name)
                        if not ret:
                            # TODO - Do Audit logging
                            self.logger.error(
                                "Object recovery queue send data "+ str(record) +
                                " failed :")
                        else:
                            self.logger.info(
                                "Object recovery queue send data successfully :" +
                                str(record))
                else:
                    self.logger.info(
                        "Index listing result empty. Ignoring adding entry to object recovery queue")
            else:
                self.logger.error("Failed to retrive Index listing:")
        except Exception as exception:
            self.logger.error(
                "add_kv_to_msgbus send data exception: {}".format(exception))
            self.logger.debug(
                "traceback : {}".format(traceback.format_exc()))

    def schedule_periodically(self):
        """Schedule producer to add key value to message_bus queue on hourly basis."""
        # Run producer periodically on hourly basis
        self.logger.info("Producer " + str(self.producer_name) + " started at : " + str(datetime.datetime.now()))
        scheduled_run = sched.scheduler(time.time, time.sleep)

        def periodic_run(scheduler):
            """Add key value to queue using scheduler."""
            if self.config.get_messaging_platform() == MESSAGE_BUS:
                self.add_kv_to_msgbus()
            else:
                self.logger.error(
                "Invalid argument specified in messaging_platform use 'message_bus'")
                return

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
    SCHEDULER = ObjectRecoveryScheduler(sys.argv[1])
    SCHEDULER.schedule_periodically()
