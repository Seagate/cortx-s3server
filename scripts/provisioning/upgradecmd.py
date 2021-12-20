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
# IMP : for upgrade cmd,
# merge_configs() is imported from the merge.py
from merge import merge_configs
import os

class UpgradeCmd(SetupCmd):
  """Upgrade Setup Cmd"""
  name = "upgrade"

  def __init__(self, config: str, services: str, commit: str = None):
    """Constructor."""
    try:
      super(UpgradeCmd, self).__init__(config, services)
    except Exception as e:
      raise e

  def process(self):
    """Main processing function."""
    self.logger.info(f"Processing phase = {self.name}, config = {self.url}, service = {self.services}")
    try:
      # TODO : Print the existing version and upgrade version
      # Existing version will come from confstore.
      # Upgrade version will come from release.info file or rpm.
      self.logger.info("validations started")
      self.phase_prereqs_validate(self.name)
      self.logger.info("validations completed")

      # check for old files before merge logic
      self.old_file_check(
            [os.path.join(self.s3_tmp_dir, "s3_cluster.yaml.sample.old")])
      if "haproxy" in self.services:
        pass
      if "s3server" in self.services:
        self.old_file_check(
            [os.path.join(self.s3_tmp_dir, "s3config.yaml.sample.old")])
      if "authserver" in self.services:
        self.old_file_check(
            [os.path.join(self.s3_tmp_dir, "keystore.properties.sample.old"),
             os.path.join(self.s3_tmp_dir, "authserver.properties.sample.old")])
      if "bgscheduler" in self.services or "bgworker" in self.services:
        self.old_file_check(
            [os.path.join(self.s3_tmp_dir, "config.yaml.sample.old")])
      # copy sample and unsafe attribute files to /etc/cortx for merge logic
      self.copy_config_files([self.get_confkey('S3_CLUSTER_CONFIG_SAMPLE_FILE'),
                              self.get_confkey('S3_CLUSTER_CONFIG_UNSAFE_ATTR_FILE')])
      # Upgrade config files
      self.logger.info(f"merge config for cluster started")
      merge_configs(
        os.path.join(self.base_config_file_path, "s3/s3backgrounddelete/s3_cluster.yaml"),
        os.path.join(self.s3_tmp_dir, "s3_cluster.yaml.sample.old"),
        os.path.join(self.base_config_file_path, "s3/s3backgrounddelete/s3_cluster.yaml.sample"),
        os.path.join(self.base_config_file_path, "s3/s3backgrounddelete/s3_cluster_unsafe_attributes.yaml"),
        'yaml://')
      self.logger.info(f"merge config for cluster complete")
      # Validating config files after upgrade
      self.validate_config_file(
             os.path.join(self.base_config_file_path, "s3/s3backgrounddelete/s3_cluster.yaml"),
             os.path.join(self.base_config_file_path, "s3/s3backgrounddelete/s3_cluster.yaml.sample"),
             'yaml://')
      # after calling merge logic, make '.old' files
      self.make_sample_old_files([self.get_confkey('S3_CLUSTER_CONFIG_SAMPLE_FILE')])
      if "haproxy" in self.services:
        pass
      if "s3server" in self.services:
        self.copy_config_files([self.get_confkey('S3_CONFIG_SAMPLE_FILE'),
                                self.get_confkey('S3_CONFIG_UNSAFE_ATTR_FILE')])
        self.logger.info(f"merge config for s3server started")
        merge_configs(
          os.path.join(self.base_config_file_path, "s3/conf/s3config.yaml"),
          os.path.join(self.s3_tmp_dir, "s3config.yaml.sample.old"),
          os.path.join(self.base_config_file_path, "s3/conf/s3config.yaml.sample"),
          os.path.join(self.base_config_file_path, "s3/conf/s3config_unsafe_attributes.yaml"),
          'yaml://')
        self.logger.info(f"merge config for s3server complete")
        self.validate_config_file(
               os.path.join(self.base_config_file_path, "s3/conf/s3config.yaml"),
               os.path.join(self.base_config_file_path, "s3/conf/s3config.yaml.sample"),
               'yaml://')
        self.make_sample_old_files([self.get_confkey('S3_CONFIG_SAMPLE_FILE')])
      if "authserver" in self.services:
        self.copy_config_files([self.get_confkey('S3_KEYSTORE_CONFIG_SAMPLE_FILE'),
                                self.get_confkey('S3_KEYSTORE_CONFIG_UNSAFE_ATTR_FILE'),
                                self.get_confkey('S3_AUTHSERVER_CONFIG_SAMPLE_FILE'),
                                self.get_confkey('S3_AUTHSERVER_CONFIG_UNSAFE_ATTR_FILE')])
        self.logger.info(f"merge config for keystore started")
        merge_configs(
          os.path.join(self.base_config_file_path, "auth/resources/keystore.properties"),
          os.path.join(self.s3_tmp_dir, "keystore.properties.sample.old"),
          os.path.join(self.base_config_file_path, "auth/resources/keystore.properties.sample"),
          os.path.join(self.base_config_file_path, "auth/resources/keystore_unsafe_attributes.properties"),
          'properties://')
        self.logger.info(f"merge config for keystore complete")
        self.logger.info(f"merge config for authserver started")
        merge_configs(
          os.path.join(self.base_config_file_path, "auth/resources/authserver.properties"),
          os.path.join(self.s3_tmp_dir, "authserver.properties.sample.old"),
          os.path.join(self.base_config_file_path, "auth/resources/authserver.properties.sample"),
          os.path.join(self.base_config_file_path, "auth/resources/authserver_unsafe_attributes.properties"),
          'properties://')
        self.logger.info(f"merge config for authserver complete")
        self.validate_config_file(
               os.path.join(self.base_config_file_path, "auth/resources/keystore.properties"),
               os.path.join(self.base_config_file_path, "auth/resources/keystore.properties.sample"),
               'properties://')
        self.validate_config_file(
               os.path.join(self.base_config_file_path, "auth/resources/authserver.properties"),
               os.path.join(self.base_config_file_path, "auth/resources/authserver.properties.sample"),
               'properties://')
        self.make_sample_old_files([self.get_confkey('S3_KEYSTORE_CONFIG_SAMPLE_FILE'),
                                    self.get_confkey('S3_AUTHSERVER_CONFIG_SAMPLE_FILE')])
      if "bgscheduler" in self.services or "bgworker" in self.services:
        self.copy_config_files([self.get_confkey('S3_BGDELETE_CONFIG_SAMPLE_FILE'),
                                self.get_confkey('S3_BGDELETE_CONFIG_UNSAFE_ATTR_FILE')])
        self.logger.info(f"merge config for background service started")
        merge_configs(
          os.path.join(self.base_config_file_path, "s3/s3backgrounddelete/config.yaml"),
          os.path.join(self.s3_tmp_dir, "config.yaml.sample.old"),
          os.path.join(self.base_config_file_path, "s3/s3backgrounddelete/config.yaml.sample"),
          os.path.join(self.base_config_file_path, "s3/s3backgrounddelete/s3backgrounddelete_unsafe_attributes.yaml"),
          'yaml://')
        self.logger.info(f"merge config for background service complete")
        self.validate_config_file(
               os.path.join(self.base_config_file_path, "s3/s3backgrounddelete/config.yaml"),
               os.path.join(self.base_config_file_path, "s3/s3backgrounddelete/config.yaml.sample"),
               'yaml://')
        self.make_sample_old_files([self.get_confkey('S3_BGDELETE_CONFIG_SAMPLE_FILE')])

    except Exception as e:
      raise S3PROVError(f'process: {self.name} failed with exception: {e}')

  def old_file_check(self, old_file_list):
    """Check for '.old' files and fail if not found."""
    for sample_old_file in old_file_list:
      if not os.path.exists(sample_old_file):
        self.logger.info(f"{sample_old_file} backup file not found")
        raise S3PROVError(f'process: {self.name} failed')
      self.logger.info(f"file is present : {sample_old_file}")

