#!/usr/bin/env python3
#
# Copyright (c) 2021 Seagate Technology LLC and/or its Affiliates
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

from setupcmd import SetupCmd, S3PROVError
from merge import merge_configs
import os
import shutil

class PostUpgradeCmd(SetupCmd):
  """Post Upgrade Setup Cmd."""
  name = "postupgrade"

  def __init__(self, config: str, module: str = None):
    """Constructor."""
    try:
      super(PostUpgradeCmd, self).__init__(config, module)
    except Exception as e:
      raise e

  def process(self):
    """Main processing function."""
    self.logger.info(f"Processing phase = {self.name}, config = {self.url}, module = {self.module}")
    try:
      self.logger.info("validations started")
      self.phase_prereqs_validate(self.name)
      self.logger.info("validations completed")

      # after rpm install, sample and unsafe attribute files needs to copy to /etc/cortx for merge logic
      self.logger.info("Copy .ample and unsafe attribute files started")
      self.copy_config_files()
      self.logger.info("Copy .ample and unsafe attribute files completed")

      # remove config file if present as they are get installed by default on /opt/seage/cortx by rpms
      self.logger.info("Delete config file started")
      self.delete_config_files()
      self.logger.info("Delete config file completed")

      # merge_configs() is imported from the merge.py
      # Upgrade config files
      self.logger.info("merge configs started")
      config_file_path = self.get_confkey('S3_TARGET_CONFIG_PATH')
      merge_configs(config_file_path)
      self.logger.info("merge configs completed")

      # Remove temporary .old files from S3 temporary location
      self.logger.info("Remove sample.old files started")
      regex = "*.sample.old"
      self.DeleteFileOrDirWithRegex(self.s3_tmp_dir, regex)
      self.logger.info("Remove sample.old files completed")

      # Validating config files after upgrade
      self.logger.info("config file validations started")
      self.validate_config_files(self.name)
      self.logger.info("config file validations completed")
    except Exception as e:
      raise S3PROVError(f'process: {self.name} failed with exception: {e}')

  def copy_config_files(self):
    """ Copy sample and unsafe attribute config files from /opt/seagate/cortx to /etc/cortx."""
    config_files = [self.get_confkey('S3_CONFIG_SAMPLE_FILE'),
                    self.get_confkey('S3_CONFIG_UNSAFE_ATTR_FILE'),
                    self.get_confkey('S3_AUTHSERVER_CONFIG_SAMPLE_FILE'),
                    self.get_confkey('S3_AUTHSERVER_CONFIG_UNSAFE_ATTR_FILE'),
                    self.get_confkey('S3_KEYSTORE_CONFIG_SAMPLE_FILE'),
                    self.get_confkey('S3_KEYSTORE_CONFIG_UNSAFE_ATTR_FILE'),
                    self.get_confkey('S3_BGDELETE_CONFIG_SAMPLE_FILE'),
                    self.get_confkey('S3_BGDELETE_CONFIG_UNSAFE_ATTR_FILE'),
                    self.get_confkey('S3_CLUSTER_CONFIG_SAMPLE_FILE'),
                    self.get_confkey('S3_CLUSTER_CONFIG_UNSAFE_ATTR_FILE')]

    # copy all the config files from the /opt/seagate/cortx to /etc/cortx
    for config_file in config_files:
      self.logger.info(f"Source config file: {config_file}")
      dest_config_file = config_file.replace("/opt/seagate/cortx", self.get_confkey('S3_TARGET_CONFIG_PATH'))
      self.logger.info(f"Dest config file: {dest_config_file}")
      os.makedirs(os.path.dirname(dest_config_file), exist_ok=True)
      shutil.move(config_file, dest_config_file)
      self.logger.info("Config file copied successfully to /etc/cortx")

  def delete_config_files(self):
    """ delete config file which are installed by rpm"""
    config_files = [self.get_confkey('S3_CONFIG_FILE'),
                self.get_confkey('S3_AUTHSERVER_CONFIG_FILE'),
                self.get_confkey('S3_KEYSTORE_CONFIG_FILE'),
                self.get_confkey('S3_BGDELETE_CONFIG_FILE'),
                self.get_confkey('S3_CLUSTER_CONFIG_FILE')]
    
    # remove config file
    for config_file in config_files:
      self.DeleteFile(config_file)
      self.logger.info(f"Config file {config_file} deleted successfully")
