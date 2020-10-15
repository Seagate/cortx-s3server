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

"""This class stores the configuration required for s3 background delete."""

import sys
import os
import shutil
import logging
import yaml

from s3backgrounddelete.cortx_cluster_config import CipherInvalidToken

try:
    from s3backgrounddelete.cortx_s3_cipher import CortxS3Cipher
except (ModuleNotFoundError, ImportError):
    # Cort-utils will not be installed in dev VM's
    pass

class CORTXS3Config(object):
    """Configuration for s3 background delete."""
    _config = None
    _conf_file = None

    def __init__(self, s3recovery_flag = False, use_cipher = True):
        """Initialise logger and configuration."""
        self.logger = logging.getLogger(__name__ + "CORTXS3Config")
        self.s3recovery_flag = s3recovery_flag
        self.s3bdg_access_key = None
        self.s3bgd_secret_key = None
        self.recovery_access_key = None
        self.recovery_secret_key = None
        self._load_and_fetch_config()
        if use_cipher:
            self.cache_credentials()

    @staticmethod
    def get_conf_dir():
        """Return configuration directory location."""
        return os.path.join(os.path.dirname(__file__), 'config')

    def _load_and_fetch_config(self):
        """Populate configuration data."""
        conf_home_dir = os.path.join(
            '/', 'opt', 'seagate', 'cortx', 's3', 's3backgrounddelete')
        self._conf_file = os.path.join(conf_home_dir, 'config.yaml')
        if not os.path.isfile(self._conf_file):
            try:
                os.stat(conf_home_dir)
            except BaseException:
                os.mkdir(conf_home_dir)
            shutil.copy(
                os.path.join(
                    self.get_conf_dir(),
                    's3_background_delete_config.yaml'),
                self._conf_file)

        if not os.access(self._conf_file, os.R_OK):
            self.logger.error(
                "Failed to read " +
                self._conf_file +
                " it doesn't have read access")
            sys.exit()
        with open(self._conf_file, 'r') as file_config:
            self._config = yaml.safe_load(file_config)

    def generate_key(self, config, use_base64, key_len, const_key):
        s3cipher = CortxS3Cipher(config, use_base64, key_len, const_key)
        return s3cipher.get_key()

    def cache_credentials(self):
        try:
            self.s3bdg_access_key = self.generate_key(None, True, 22, "s3backgroundaccesskey")
        except CipherInvalidToken as err:
            self.s3bdg_access_key = None
            self.logger.info("S3cipher failed due to "+ str(err) +". Using credentails from config file")

        try:
            self.s3bgd_secret_key = self.generate_key(None, False, 40, "s3backgroundsecretkey")
        except CipherInvalidToken as err:
            self.s3bgd_secret_key = None
            self.logger.info("S3cipher failed due to "+ str(err) +". Using credentails from config file")

        try:
            self.recovery_access_key = self.generate_key(None, True, 22, "s3recoveryaccesskey")
        except CipherInvalidToken as err:
            self.recovery_access_key = None
            self.logger.info("S3cipher failed due to "+ str(err) +". Using credentails from config file")

        try:
            self.recovery_secret_key = self.generate_key(None, False, 40, "s3recoverysecretkey")
        except CipherInvalidToken as err:
            self.recovery_secret_key = None
            self.logger.info("S3cipher failed due to "+ str(err) +". Using credentails from config file")


    def get_s3recovery_flag(self):
        return self.s3recovery_flag

    def get_config_version(self):
        """Return version of S3 background delete config file or KeyError."""
        if 'version_config' in self._config and self._config['version_config']['version']:
            return self._config['version_config']['version']
        else:
            raise KeyError("Could not parse version from config file " + self._conf_file)

    def get_logger_directory(self):
        """Return logger directory path for background delete from config file or KeyError."""
        if 'logconfig' in self._config and self._config['logconfig']['logger_directory']:
            return self._config['logconfig']['logger_directory']
        else:
            raise KeyError(
                "Could not parse logger directory path from config file " +
                self._conf_file)

    def get_scheduler_logger_name(self):
        """Return logger name for scheduler from config file or KeyError."""
        if 'logconfig' in self._config and self._config['logconfig']['scheduler_logger_name']:
            return self._config['logconfig']['scheduler_logger_name']
        else:
            raise KeyError(
                "Could not parse scheduler loggername from config file " +
                self._conf_file)

    def get_processor_logger_name(self):
        """Return logger name for processor from config file or KeyError."""
        if 'logconfig' in self._config and self._config['logconfig']['processor_logger_name']:
            return self._config['logconfig']['processor_logger_name']
        else:
            raise KeyError(
                "Could not parse processor loggername from config file " +
                self._conf_file)

    def get_scheduler_logger_file(self):
        """Return logger file for scheduler from config file or KeyError."""
        if 'logconfig' in self._config and self._config['logconfig']['scheduler_log_file']:
            return self._config['logconfig']['scheduler_log_file']
        else:
            raise KeyError(
                "Could not parse scheduler logfile from config file " +
                self._conf_file)

    def get_processor_logger_file(self):
        """Return logger file for processor from config file or KeyError."""
        if 'logconfig' in self._config and self._config['logconfig']['processor_log_file']:
            return self._config['logconfig']['processor_log_file']
        else:
            raise KeyError(
                "Could not parse processor loggerfile from config file " +
                self._conf_file)

    def get_file_log_level(self):
        """Return file log level from config file or KeyError."""
        if 'logconfig' in self._config and self._config['logconfig']['file_log_level']:
            return self._config['logconfig']['file_log_level']
        else:
            raise KeyError(
                "Could not parse file loglevel from config file " +
                self._conf_file)

    def get_console_log_level(self):
        """Return console log level from config file or KeyError."""
        if 'logconfig' in self._config and self._config['logconfig']['console_log_level']:
            return self._config['logconfig']['console_log_level']
        else:
            raise KeyError(
                "Could not parse console loglevel from config file " +
                self._conf_file)

    def get_log_format(self):
        """Return log format from config file or KeyError."""
        if 'logconfig' in self._config and self._config['logconfig']['log_format']:
            return self._config['logconfig']['log_format']
        else:
            raise KeyError(
                "Could not parse log format from config file " +
                self._conf_file)

    def get_cortx_s3_endpoint(self):
        """Return endpoint from config file or KeyError."""
        if 'cortx_s3' in self._config and self._config['cortx_s3']['endpoint']:
            return self._config['cortx_s3']['endpoint']
        else:
            raise KeyError(
                "Could not find cortx_s3 endpoint from config file " +
                self._conf_file)

    def get_cortx_s3_service(self):
        """Return service from config file or KeyError."""
        if 'cortx_s3' in self._config and self._config['cortx_s3']['service']:
            return self._config['cortx_s3']['service']
        else:
            raise KeyError(
                "Could not find cortx_s3 service from config file " +
                self._conf_file)

    def get_cortx_s3_region(self):
        """Return region from config file or KeyError."""
        if 'cortx_s3' in self._config and self._config['cortx_s3']['default_region']:
            return self._config['cortx_s3']['default_region']
        else:
            raise KeyError(
                "Could not find cortx_s3 default_region from config file " +
                self._conf_file)

    def get_cortx_s3_access_key(self):
        """Return access_key cipher or config file or KeyError."""
        if self.s3bdg_access_key:
            return self.s3bdg_access_key

        if 'cortx_s3' in self._config and self._config['cortx_s3']['background_account_access_key']:
            return self._config['cortx_s3']['background_account_access_key']
        else:
            raise KeyError(
                "Could not find cortx_s3 access_key from config file " +
                self._conf_file)

    def get_cortx_s3_secret_key(self):
        """Return secret_key from cipher or config file or KeyError."""
        if self.s3bgd_secret_key:
            return self.s3bgd_secret_key

        if 'cortx_s3' in self._config and self._config['cortx_s3']['background_account_secret_key']:
            return self._config['cortx_s3']['background_account_secret_key']
        else:
            raise KeyError(
                "Could not find cortx_s3 secret_key from config file " +
                self._conf_file)

    def get_daemon_mode(self):
        """Return daemon_mode flag value for scheduler from config file\
           else it should return default as "True"."""
        if 'cortx_s3' in self._config and 'daemon_mode' in self._config['cortx_s3']:
            return self._config['cortx_s3']['daemon_mode']
        else:
            #Return default mode as daemon mode i.e. "True"
            return True

    def get_rabbitmq_username(self):
        """Return username of rabbitmq from config file or KeyError."""
        if 'rabbitmq' in self._config and self._config['rabbitmq']['username']:
            return self._config['rabbitmq']['username']
        else:
            raise KeyError(
                "Could not parse rabbitmq username from config file " +
                self._conf_file)

    def get_rabbitmq_password(self):
        """Return password of rabbitmq from config file or KeyError."""
        if 'rabbitmq' in self._config and self._config['rabbitmq']['password']:
            return self._config['rabbitmq']['password']
        else:
            raise KeyError(
                "Could not parse rabbitmq password from config file " +
                self._conf_file)

    def get_rabbitmq_host(self):
        """Return host of rabbitmq from config file or KeyError."""
        if 'rabbitmq' in self._config and self._config['rabbitmq']['host']:
            return self._config['rabbitmq']['host']
        else:
            raise KeyError(
                "Could not parse rabbitmq host from config file " +
                self._conf_file)

    def get_rabbitmq_queue_name(self):
        """Return queue name of rabbitmq from config file or KeyError."""
        if 'rabbitmq' in self._config and self._config['rabbitmq']['queue']:
            return self._config['rabbitmq']['queue']
        else:
            raise KeyError(
                "Could not parse rabbitmq queue from config file " +
                self._conf_file)

    def get_rabbitmq_exchange(self):
        """
        Return exchange name of rabbitmq from config file.
        The exchange parameter is the name of the exchange.
        The empty string denotes the default or nameless exchange messages are
        routed to the queue with the name specified by routing_key,if it exists
        """
        return self._config['rabbitmq']['exchange']

    def get_rabbitmq_exchange_type(self):
        """Return exchange type of rabbitmq from config file or KeyError."""
        if 'rabbitmq' in self._config and self._config['rabbitmq']['exchange_type']:
            return self._config['rabbitmq']['exchange_type']
        else:
            raise KeyError(
                "Could not parse rabbitmq exchange_type from config file " +
                self._conf_file)

    def get_rabbitmq_mode(self):
        """Return mode of rabbitmq from config file or KeyError."""
        if 'rabbitmq' in self._config and self._config['rabbitmq']['mode']:
            return self._config['rabbitmq']['mode']
        else:
            raise KeyError(
                "Could not parse rabbitmq mode from config file " +
                self._conf_file)

    def get_rabbitmq_durable(self):
        """Return durable of rabbitmq from config file or KeyError."""
        if 'rabbitmq' in self._config and self._config['rabbitmq']['durable']:
            return self._config['rabbitmq']['durable']
        else:
            raise KeyError(
                "Could not parse rabbitmq durable from config file " +
                self._conf_file)

    def get_schedule_interval(self):
        """Return schedule interval of rabbitmq from config file or KeyError."""
        if 'rabbitmq' in self._config and self._config['rabbitmq']['schedule_interval_secs']:
            return self._config['rabbitmq']['schedule_interval_secs']
        else:
            raise KeyError(
                "Could not parse rabbitmq schedule interval from config file " +
                self._conf_file)

    def get_probable_delete_index_id(self):
        """Return probable delete index-id from config file or KeyError."""
        if 'indexid' in self._config and self._config['indexid']['probable_delete_index_id']:
            return self._config['indexid']['probable_delete_index_id']
        else:
            raise KeyError(
                "Could not parse probable delete index-id from config file " +
                self._conf_file)

    def get_max_keys(self):
        """Return maximum number of keys from config file or KeyError."""
        if 'indexid' in self._config and self._config['indexid']['max_keys']:
            return self._config['indexid']['max_keys']
        else:
            return 1000

    def get_global_instance_index_id(self):
        """Return probable delete index-id from config file or KeyError."""
        if 'indexid' in self._config and self._config['indexid']['global_instance_index_id']:
            return self._config['indexid']['global_instance_index_id']
        else:
            raise KeyError(
                "Could not parse global instance index-id from config file " +
                self._conf_file)

    def get_max_bytes(self):
        """Return maximum bytes for a log file"""
        if 'logconfig' in self._config and self._config['logconfig']['max_bytes']:
            return self._config['logconfig']['max_bytes']
        else:
            raise KeyError(
                "Could not parse maxBytes from config file " +
                self._conf_file)

    def get_backup_count(self):
        """Return count of log files"""
        if 'logconfig' in self._config and self._config['logconfig']['backup_count']:
            return self._config['logconfig']['backup_count']
        else:
            raise KeyError(
                "Could not parse backupcount from config file " +
                self._conf_file)

    def get_leak_processing_delay_in_mins(self):
        """Return 'leak_processing_delay_in_mins' from 'leakconfig' section """
        if 'leakconfig' in self._config and 'leak_processing_delay_in_mins' in self._config['leakconfig']:
            return self._config['leakconfig']['leak_processing_delay_in_mins']
        else:
            # default delay is 15mins
            return 15

    def get_version_processing_delay_in_mins(self):
        """Return 'version_processing_delay_in_mins' from 'leakconfig' section """
        if 'leakconfig' in self._config and 'version_processing_delay_in_mins' in self._config['leakconfig']:
            return self._config['leakconfig']['version_processing_delay_in_mins']
        else:
            # default delay is 15mins
            return 15

    def get_global_bucket_index_id(self):
        """Return probable delete index-id from config file or KeyError."""
        if 'indexid' in self._config and self._config['indexid']['global_bucket_index_id']:
            return self._config['indexid']['global_bucket_index_id']
        else:
            raise KeyError(
                "Could not parse global_bucket index-id from config file " +
                self._conf_file)

    def get_bucket_metadata_index_id(self):
        """Return probable delete index-id from config file or KeyError."""
        if 'indexid' in self._config and self._config['indexid']['bucket_metadata_index_id']:
            return self._config['indexid']['bucket_metadata_index_id']
        else:
            raise KeyError(
                "Could not parse bucket_metadata index_id from config file " +
                self._conf_file)

    def get_s3_instance_count(self):
        """Return secret_key from config file or KeyError."""
        if 'cortx_s3' in self._config and self._config['cortx_s3']['s3_instance_count']:
            return self._config['cortx_s3']['s3_instance_count']
        else:
            raise KeyError(
                "Could not find s3_instance_count from config file " +
                self._conf_file)

    def get_s3_recovery_access_key(self):
        """Return access_key from cipher or config file or KeyError."""
        if self.recovery_access_key:
            return self.recovery_access_key

        if 's3_recovery' in self._config and self._config['s3_recovery']['recovery_account_access_key']:
            return self._config['s3_recovery']['recovery_account_access_key']
        else:
            raise KeyError(
                "Could not find s3_recovery access_key from config file " +
                self._conf_file)

    def get_s3_recovery_secret_key(self):
        """Return secret_key from cipher or config file or KeyError."""
        if self.recovery_secret_key:
            return self.recovery_secret_key

        if 's3_recovery' in self._config and self._config['s3_recovery']['recovery_account_secret_key']:
            return self._config['s3_recovery']['recovery_account_secret_key']
        else:
            raise KeyError(
                "Could not find s3_recovery secret_key from config file " +
                self._conf_file)
