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
This class acts as object recovery processor which consumes
the cortx message_bus message queue.
"""
#!/usr/bin/python3.6

import os
import errno
import traceback
import logging
import datetime
import signal
from logging import handlers

from s3backgrounddelete.cortx_s3_config import CORTXS3Config
from s3backgrounddelete.cortx_s3_signal import DynamicConfigHandler
from s3backgrounddelete.cortx_s3_signal import SigTermHandler
from s3backgrounddelete.cortx_s3_constants import MESSAGE_BUS
from s3msgbus.cortx_s3_msgbus import S3CortxMsgBus
from cortx.utils.log import Log

class ObjectRecoveryProcessor(object):
    """Provides consumer for object recovery"""

    def __init__(self,base_config_path:str = "/etc/cortx",config_type:str = "yaml://"):
        """Initialise Server, config and create logger."""
        self.server = None
        self.config = CORTXS3Config(base_cfg_path = base_config_path,cfg_type = config_type, log_init=False)
        self.create_logger_directory()
        Log.init(self.config.get_processor_logger_name(),
                 self.config.get_processor_logger_directory(),
                 level=self.config.get_file_log_level(),
                 backup_count=self.config.get_backup_count(),
                 file_size_in_mb=self.config.get_max_log_size_mb(),
                 syslog_server=None, syslog_port=None,
                 console_output=True)
        self.signal = DynamicConfigHandler(self)
        Log.info("Initialising the Object Recovery Processor")
        if self.config.get_messaging_platform() == MESSAGE_BUS:
          endpoints_val = self.config.get_msgbus_platform_url()
          S3CortxMsgBus.configure_endpoint(endpoints_val)
        self.term_signal = SigTermHandler()

    def consume(self):
        """Consume the objects from object recovery queue."""
        self.server = None
        try:
            #Conditionally importing ObjectRecoveryMsgbusConsumer when config setting says so.
            if self.config.get_messaging_platform() == MESSAGE_BUS:
                from s3backgrounddelete.object_recovery_msgbus import ObjectRecoveryMsgbus

                self.server = ObjectRecoveryMsgbus(
                    self.config)
            else:
                Log.error(
                "Invalid argument : " + self.config.get_messaging_platform() + "specified in messaging_platform.")
                return

            Log.info("Consumer started at " +
                            str(datetime.datetime.now()))
            self.server.receive_data(self.term_signal)
        except BaseException:
            if self.server:
                self.server.close()
            Log.error("main except:" + str(traceback.format_exc()))

    def close(self):
        """Stop processor."""
        Log.info("Stopping the processor")
        self.server.close()
        # perform an orderly shutdown by flushing and closing all handlers
        logging.shutdown()

    def create_logger_directory(self):
        """Create log directory if not exsists."""
        self._logger_directory = os.path.join(self.config.get_processor_logger_directory())
        if not os.path.isdir(self._logger_directory):
            try:
                os.mkdir(self._logger_directory)
            except OSError as e:
                if e.errno == errno.EEXIST:
                    pass
                else:
                    raise Exception("Consumer Logger Could not be created")

if __name__ == "__main__":
    PROCESSOR = ObjectRecoveryProcessor()
    PROCESSOR.consume()
