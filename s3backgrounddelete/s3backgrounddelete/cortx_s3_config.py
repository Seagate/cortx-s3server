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
import uuid

from s3backgrounddelete.cortx_cluster_config import CipherInvalidToken
from s3confstore.cortx_s3_confstore import S3CortxConfStore

try:
    from s3cipher.cortx_s3_cipher import CortxS3Cipher
except (ModuleNotFoundError, ImportError):
    # Cort-utils will not be installed in dev VM's
    pass

class CORTXS3Config(object):
    """Configuration for s3 background delete."""
    _config = None
    _conf_file = None
    s3confstore = None

    def __init__(self):
        """Initialise logger and configuration."""
        self.logger = logging.getLogger(__name__ + "CORTXS3Config")
        self.s3bdg_access_key = None
        self.s3bgd_secret_key = None
        self._load_and_fetch_config()
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
        # Load config.yaml file through confstore.
        self._conf_file ='yaml://' + self._conf_file
        self.s3confstore = S3CortxConfStore(config=self._conf_file, index= str(uuid.uuid1()))
        
    def generate_key(self, config, use_base64, key_len, const_key):
        s3cipher = CortxS3Cipher(config, use_base64, key_len, const_key)
        return s3cipher.generate_key()

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

    def get_config_version(self):
        """Return version of S3 background delete config file or KeyError."""
        try:
          config_version = self.s3confstore.get_config('version_config>version')
          return int(config_version)
        except:
            raise KeyError("Could not parse version from config file " + self._conf_file)

    def get_logger_directory(self):
        """Return logger directory path for background delete from config file or KeyError."""
        try:
          log_directory = self.s3confstore.get_config('logconfig>logger_directory')
          return log_directory
        except:
            raise KeyError(
                "Could not parse logger directory path from config file " +
                self._conf_file)

    def get_scheduler_logger_name(self):
        """Return logger name for scheduler from config file or KeyError."""
        try:
          scheduler_log_name = self.s3confstore.get_config('logconfig>scheduler_logger_name')
          return scheduler_log_name
        except:
            raise KeyError(
                "Could not parse scheduler loggername from config file " +
                self._conf_file)

    def get_processor_logger_name(self):
        """Return logger name for processor from config file or KeyError."""
        try:
          processor_log_name = self.s3confstore.get_config('logconfig>processor_logger_name')
          return processor_log_name
        except:
            raise KeyError(
                "Could not parse processor loggername from config file " +
                self._conf_file)

    def get_scheduler_logger_file(self):
        """Return logger file for scheduler from config file or KeyError."""
        try:
          scheduler_log_file = self.s3confstore.get_config('logconfig>scheduler_log_file')
          return scheduler_log_file
        except:
            raise KeyError(
                "Could not parse scheduler logfile from config file " +
                self._conf_file)

    def get_processor_logger_file(self):
        """Return logger file for processor from config file or KeyError."""
        try:
          processor_log_file = self.s3confstore.get_config('logconfig>processor_log_file')
          return processor_log_file
        except:
            raise KeyError(
                "Could not parse processor loggerfile from config file " +
                self._conf_file)


    def get_file_log_level(self):
        """Return file log level from config file or KeyError."""
        try:
          log_level = self.s3confstore.get_config('logconfig>file_log_level')
          return int(log_level)
        except:
            raise KeyError(
                "Could not parse file loglevel from config file " +
                self._conf_file)

    def get_console_log_level(self):
        """Return console log level from config file or KeyError."""
        try:
          console_log_level = self.s3confstore.get_config('logconfig>console_log_level')
          return int(console_log_level)
        except:
            raise KeyError(
                "Could not parse console loglevel from config file " +
                self._conf_file)

    def get_log_format(self):
        """Return log format from config file or KeyError."""
        try:
          log_format = self.s3confstore.get_config('logconfig>log_format')
          return log_format
        except:
            raise KeyError(
                "Could not parse log format from config file " +
                self._conf_file)

    def get_cortx_s3_endpoint(self):
        """Return endpoint from config file or KeyError."""
        try:
            cortx_s3_endpoint = self.s3confstore.get_config('cortx_s3>endpoint')
            return cortx_s3_endpoint
        except:
            raise KeyError(
                "Could not find cortx_s3 endpoint from config file " +
                self._conf_file)

    def get_cortx_s3_endpoint_for_consumer(self):
        """Return endpoint from config file or KeyError."""
        try:
            #change from cortx_s3>endpoint to cortx_s3>consumer_endpoint
            cortx_s3_endpoint = self.s3confstore.get_config('cortx_s3>consumer_endpoint')
            return cortx_s3_endpoint
        except:
            raise KeyError(
                "Could not find cortx_s3 endpoint from config file " +
                self._conf_file)

    def get_cortx_s3_endpoint_for_producer(self):
        """Return endpoint from config file or KeyError."""
        try:
            #change from cortx_s3>endpoint to cortx_s3>producer_endpoint
            cortx_s3_endpoint = self.s3confstore.get_config('cortx_s3>producer_endpoint')
            return cortx_s3_endpoint
        except:
            raise KeyError(
                "Could not find cortx_s3 endpoint from config file " +
                self._conf_file)

    def get_cortx_s3_service(self):
        """Return service from config file or KeyError."""
        try:
          cortx_s3_service = self.s3confstore.get_config('cortx_s3>service')
          return cortx_s3_service
        except:
            raise KeyError(
                "Could not find cortx_s3 service from config file " +
                self._conf_file)

    def get_cortx_s3_region(self):
        """Return region from config file or KeyError."""
        try:
          cortx_s3_region = self.s3confstore.get_config('cortx_s3>default_region')
          return cortx_s3_region
        except:
            raise KeyError(
                "Could not find cortx_s3 default_region from config file " +
                self._conf_file)

    def get_cortx_s3_access_key(self):
        """Return access_key cipher or config file or KeyError."""
        if self.s3bdg_access_key:
            return self.s3bdg_access_key

        raise KeyError(
            "Could not find s3bdg_access_key")

    def get_cortx_s3_secret_key(self):
        """Return secret_key from cipher or config file or KeyError."""
        if self.s3bgd_secret_key:
            return self.s3bgd_secret_key

        raise KeyError(
            "Could not find s3bgd_secret_key")

    def get_daemon_mode(self):
        """Return daemon_mode flag value for scheduler from config file\
           else it should return default as "True"."""
        daemon_mode = self.s3confstore.get_config('cortx_s3>daemon_mode')
        if not daemon_mode:
            return daemon_mode
        else:
            #Return default mode as daemon mode i.e. "True"
            return True

    def get_schedule_interval(self):
        """Return schedule interval of object recovery scheduler from config file or KeyError."""
        try:
          schedule_interval = self.s3confstore.get_config('cortx_s3>scheduler_schedule_interval')
          return int(schedule_interval)
        except:
            raise KeyError(
                "Could not parse schedule interval for object recovery scheduler from config file " +
                self._conf_file)

    def get_probable_delete_index_id(self):
        """Return probable delete index-id from config file or KeyError."""
        try:
          probable_delete_index_id = self.s3confstore.get_config('indexid>probable_delete_index_id')
          return probable_delete_index_id
        except:
            raise KeyError(
                "Could not parse probable delete index-id from config file " +
                self._conf_file)

    def get_max_keys(self):
        """Return maximum number of keys from config file or KeyError."""
        max_keys = self.s3confstore.get_config('indexid>max_keys')
        if max_keys is not None:
            return int(max_keys)
        else:
            raise Exception(f'Failed to read indexid>max_keys: '
                        f'{max_keys}\n')

    def get_threshold(self):
        """Return the threshold for max."""
        threshold = self.s3confstore.get_config('indexid>threshold')
        if threshold is not None:
            return int(threshold)
        else:
            raise Exception(f'Failed to read indexid>threshold: '
                        f'{threshold}\n')
                        

    def get_global_instance_index_id(self):
        """Return probable delete index-id from config file or KeyError."""
        try:
          global_instance_index_id = self.s3confstore.get_config('indexid>global_instance_index_id')
          return global_instance_index_id
        except:
            raise KeyError(
                "Could not parse global instance index-id from config file " +
                self._conf_file)

    def get_max_bytes(self):
        """Return maximum bytes for a log file"""
        try:
          max_bytes = self.s3confstore.get_config('logconfig>max_bytes')
          return int(max_bytes)
        except:
            raise KeyError(
                "Could not parse maxBytes from config file " +
                self._conf_file)

    def get_backup_count(self):
        """Return count of log files"""
        try:
          backup_count = self.s3confstore.get_config('logconfig>backup_count')
          return int(backup_count)
        except:
            raise KeyError(
                "Could not parse backupcount from config file " +
                self._conf_file)

    def get_leak_processing_delay_in_mins(self):
        """Return 'leak_processing_delay_in_mins' from 'leakconfig' section """
        leak_processing_delay_in_mins = self.s3confstore.get_config('leakconfig>leak_processing_delay_in_mins')
        if leak_processing_delay_in_mins is not None:
            return int(leak_processing_delay_in_mins)
        else:
            raise Exception(f'Failed to read leakconfig>leak_processing_delay_in_mins: '
                        f'{leak_processing_delay_in_mins}\n')

    def get_version_processing_delay_in_mins(self):
        """Return 'version_processing_delay_in_mins' from 'leakconfig' section """
        version_processing_delay_in_mins = self.s3confstore.get_config('leakconfig>version_processing_delay_in_mins')
        if version_processing_delay_in_mins is not None:
            return int(version_processing_delay_in_mins)
        else:
            raise Exception(f'Failed to read leakconfig>version_processing_delay_in_mins: '
                        f'{version_processing_delay_in_mins}\n')

    def get_global_bucket_index_id(self):
        """Return probable delete index-id from config file or KeyError."""
        try:
          global_bucket_index_id = self.s3confstore.get_config('indexid>global_bucket_index_id')
          return global_bucket_index_id
        except:
            raise KeyError(
                "Could not parse global_bucket index-id from config file " +
                self._conf_file)

    def get_bucket_metadata_index_id(self):
        """Return probable delete index-id from config file or KeyError."""
        try:
          bucket_metadata_index_id = self.s3confstore.get_config('indexid>bucket_metadata_index_id')
          return bucket_metadata_index_id
        except:
            raise KeyError(
                "Could not parse bucket_metadata index_id from config file " +
                self._conf_file)

    def get_s3_instance_count(self):
        """Return secret_key from config file or KeyError."""
        try:
          s3_instance_count = self.s3confstore.get_config('cortx_s3>s3_instance_count')
          return int(s3_instance_count)
        except:
            raise KeyError(
                "Could not find s3_instance_count from config file " +
                self._conf_file)

    def get_s3_recovery_access_key(self):
        """Return access_key from cipher or config file or KeyError."""
        if self.recovery_access_key:
            return self.recovery_access_key

        raise KeyError(
            "Could not find s3_recovery access_key")

    def get_s3_recovery_secret_key(self):
        """Return secret_key from cipher or config file or KeyError."""
        if self.recovery_secret_key:
            return self.recovery_secret_key

        raise KeyError(
                "Could not find s3_recovery secret_key")
        

    def get_cleanup_enabled(self):
        """Return flag cleanup_enabled for S3 non active"""
        try:
            cleanup_enabled = self.s3confstore.get_config('leakconfig>cleanup_enabled')
            return cleanup_enabled
        except KeyError:
            # Default value used for S/W update
            return False

    def get_messaging_platform(self):
        """Return use_msgbus from config file or False"""
        try:
          messaging_platform = self.s3confstore.get_config('cortx_s3>messaging_platform')
          return messaging_platform
        except:
            raise KeyError(
                "Could not parse messaging_platform from config file " +
                self._conf_file)

    def get_msgbus_topic(self):
        """Return topic of msgbus from config file or KeyError."""
        try:
          msgbus_topic = self.s3confstore.get_config('message_bus>topic')
          return msgbus_topic
        except:
            raise KeyError(
                "Could not parse topic from config file " +
                self._conf_file)

    def get_msgbus_consumer_group(self):
        """Return consumer group id from config file or KeyError."""
        try:
          msgbus_consumer_group = self.s3confstore.get_config('message_bus>consumer_group')
          return msgbus_consumer_group
        except:
            raise KeyError(
                "Could not parse consumer_group from config file " +
                self._conf_file)

    def get_msgbus_consumer_id_prefix(self):
        """Return consumer id prefix from config file or KeyError."""
        try:
          msgbus_consumer_id_prefix = self.s3confstore.get_config('message_bus>consumer_id_prefix')
          return msgbus_consumer_id_prefix
        except:
            raise KeyError(
                "Could not parse consumer_id_prefix from config file " +
                self._conf_file)

    def get_msgbus_consumer_sleep_time(self):
        """Return consumer sleep time from config file or KeyError."""
        try:
          msgbus_consumer_sleep_time = self.s3confstore.get_config('message_bus>consumer_sleep')
          return int(msgbus_consumer_sleep_time)
        except:
            raise KeyError(
                "Could not parse consumer_sleep from config file " +
                self._conf_file)

    def get_msgbus_producer_id(self):
        """Return producer_id prefix from config file or KeyError."""
        try:
          msgbus_producer_id = self.s3confstore.get_config('message_bus>producer_id')
          return msgbus_producer_id
        except:
            raise KeyError(
                "Could not parse producer_id from config file " +
                self._conf_file)

    def get_msgbus_producer_delivery_mechanism(self):
        """Return producer delivery mechanism from config file or KeyError."""
        try:
          msgbus_producer_delivery_mechanism = self.s3confstore.get_config('message_bus>producer_delivery_mechanism')
          return msgbus_producer_delivery_mechanism
        except:
            raise KeyError(
                "Could not parse producer_delivery_mechanism from config file " +
                self._conf_file)

    def get_msgbus_admin_id(self):
        """Return admin id from config file or KeyError."""
        try:
          msgbus_admin_id = self.s3confstore.get_config('message_bus>admin_id')
          return msgbus_admin_id
        except:
            raise KeyError(
                "Could not parse admin id from config file " +
                self._conf_file)

    def get_purge_sleep_time(self):
        """Return purge sleep time from config file or KeyError."""
        try:
          msgbus_purge_sleep = self.s3confstore.get_config('message_bus>purge_sleep')
          return int(msgbus_purge_sleep)
        except:
            raise KeyError(
                "Could not parse purge sleep from config file " +
                self._conf_file)