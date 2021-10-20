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
This class implements the signal handler that is used for
dynamically changing config parameters
"""
#!/usr/bin/python3.6

import os
import traceback
import logging
import datetime
import signal
from logging import handlers
from functools import partial

from s3backgrounddelete.cortx_s3_config import CORTXS3Config

class DynamicConfigHandler(object):
    """Signal handler class for dynamically changing config parameters"""

    def __init__(self,objectx):
        """Init the signal handler"""
        sighupArg=objectx
        signal.signal(signal.SIGHUP,partial(self.sighup_handler_callback, sighupArg))

    def sighup_handler_callback(self, sighupArg, signum, frame):
        """Reload the configuration"""
        sighupArg.config = CORTXS3Config()
        sighupArg.logger.setLevel(sighupArg.config.get_file_log_level())

        sighupArg.logger.info("Logging level has been changed")

class SigTermHandler(object):
    """SigTerm Signal handler class for exiting gracefully"""

    shutdown_signal = False

    def __init__(self):
        """Init the signal handler"""
        signal.signal(signal.SIGTERM,partial(self.sigterm_handler_callback))

    def sigterm_handler_callback(self, *args):
        """Set shutdown"""
        self.shutdown_signal = True
