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
import errno
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
from s3backgrounddelete.cortx_s3_constants import CONNECTION_TYPE_PRODUCER
from s3backgrounddelete.cortx_s3_signal import SigTermHandler
from s3backgrounddelete.cortx_s3_constants import MESSAGE_BUS
from s3msgbus.cortx_s3_msgbus import S3CortxMsgBus
from cortx.utils.log import Log
#from s3backgrounddelete.IEMutil import IEMutil

class ObjectRecoveryScheduler(object):
    """Scheduler which will add key value to message_bus queue."""

    def __init__(self, producer_name:str,base_config_path:str = "/etc/cortx",config_type:str = "yaml://"):
        """Initialise logger and configuration."""
        self.data = None
        self.config = CORTXS3Config(base_cfg_path = base_config_path,cfg_type = config_type, log_init=False)
        self.create_logger_directory()
        Log.init(self.config.get_scheduler_logger_name(),
                 self.config.get_scheduler_logger_directory(),
                 level=self.config.get_file_log_level(),
                 backup_count=self.config.get_backup_count(),
                 file_size_in_mb=self.config.get_max_log_size_mb(),
                 syslog_server=None, syslog_port=None,
                 console_output=True)
        self.signal = DynamicConfigHandler(self)
        Log.info("Initialising the Object Recovery Scheduler")
        if self.config.get_messaging_platform() == MESSAGE_BUS:
          endpoints_val = self.config.get_msgbus_platform_url()
          S3CortxMsgBus.configure_endpoint(endpoints_val)
        self.producer = None
        self.producer_name = producer_name
        self.term_signal = SigTermHandler()

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
        Log.info("Inside add_kv_to_msgbus.")
        try:
            from s3backgrounddelete.object_recovery_msgbus import ObjectRecoveryMsgbus

            if not self.producer:
                self.producer = ObjectRecoveryMsgbus(
                    self.config)
            if self.term_signal.shutdown_signal == True:
                Log.info("Shutting down s3backgroundproducer service.")
                sys.exit(0)

            self.producer.purge()
            result, index_response = CORTXS3IndexApi(
                self.config, connectionType=CONNECTION_TYPE_PRODUCER).list(
                    self.config.get_probable_delete_index_id(), self.config.get_max_keys(), marker)
            if result and not self.term_signal.shutdown_signal:
                Log.info("Index listing result :" +
                                 str(index_response.get_index_content()))
                probable_delete_json = index_response.get_index_content()
                probable_delete_oid_list = probable_delete_json["Keys"]
                is_truncated = probable_delete_json["IsTruncated"]
                if (probable_delete_oid_list is not None and not self.term_signal.shutdown_signal):
                    for record in probable_delete_oid_list:
                        # Check if record is older than the pre-configured 'time to process' delay
                        leak_processing_delay = self.config.get_leak_processing_delay_in_mins()
                        try:
                            objLeakVal = json.loads(record["Value"])
                        except ValueError as error:
                            Log.error(
                            "Failed to parse JSON data for: " + str(record) + " due to: " + error)
                            continue

                        if (objLeakVal is None):
                            Log.error("No value associated with " + str(record) + ". Skipping entry")
                            continue

                        # Check if object leak entry is older than 15mins or a preconfigured duration
                        if (not ObjectRecoveryScheduler.isObjectLeakEntryOlderThan(objLeakVal, leak_processing_delay)):
                            Log.info("Object leak entry " + record["Key"] +
                                            " is NOT older than " + str(leak_processing_delay) +
                                            "mins. Skipping entry")
                            continue

                        Log.info(
                            "Object recovery queue sending data :" +
                            str(record))
                        ret = self.producer.send_data(record, producer_id = self.producer_name)
                        if not ret:
                            # TODO - Do Audit logging
                            Log.error(
                                "Object recovery queue send data "+ str(record) +
                                " failed :")
                        else:
                            Log.info(
                                "Object recovery queue send data successfully :" +
                                str(record))
                else:
                    Log.info(
                        "Index listing result empty. Ignoring adding entry to object recovery queue")
            else:
                Log.error("Failed to retrive Index listing:")
        except Exception as exception:
            Log.error(
                "add_kv_to_msgbus send data exception: {}".format(exception))
            Log.debug(
                "traceback : {}".format(traceback.format_exc()))

    def schedule_periodically(self):
        """Schedule producer to add key value to message_bus queue on hourly basis."""
        # Run producer periodically on hourly basis
        Log.info("Producer " + str(self.producer_name) + " started at : " + str(datetime.datetime.now()))
        scheduled_run = sched.scheduler(time.time, time.sleep)

        def one_sec_run(scheduler):
            pass

        def divide_interval():
            for _ in range(int(self.config.get_schedule_interval()) - 1):
                if self.term_signal.shutdown_signal == True:
                    break
                scheduled_run.enter(1,
                                   1, one_sec_run, (scheduled_run,))
                scheduled_run.run()

        def periodic_run(scheduler):
            """Add key value to queue using scheduler."""
            if self.config.get_messaging_platform() == MESSAGE_BUS:
                self.add_kv_to_msgbus()
            else:
                Log.error(
                "Invalid argument specified in messaging_platform use 'message_bus'")
                return
            divide_interval()
            scheduled_run.enter(1,
                           1, periodic_run, (scheduled_run,))

        divide_interval()
        scheduled_run.enter(1,
                           1, periodic_run, (scheduled_run,))
        scheduled_run.run()

    def create_logger_directory(self):
        """Create log directory if not exsists."""
        self._logger_directory = os.path.join(self.config.get_scheduler_logger_directory())
        if not os.path.isdir(self._logger_directory):
            try:
                os.mkdir(self._logger_directory)
            except OSError as e:
                if e.errno == errno.EEXIST:
                    pass
                else:
                    raise Exception(" Producer Logger Could not be created")


if __name__ == "__main__":
    SCHEDULER = ObjectRecoveryScheduler(sys.argv[1])
    SCHEDULER.schedule_periodically()
