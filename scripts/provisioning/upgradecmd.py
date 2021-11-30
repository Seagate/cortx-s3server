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

class UpgradeCmd(SetupCmd):

  """Upgrade Setup Cmd"""
  name = "upgrade"

  def __init__(self, config: str, services: str, commit: str = None):
    """Constructor."""
    try:
      super(UpgradeCmd, self).__init__(config, services, commit)
    except Exception as e:
      raise e

  def process(self):
    """Main processing function."""
    self.logger.info(f"Processing phase = {self.name}, config = {self.url}, service = {self.services}, commit = {self.commit}")
    try:
      # TODO : Print the existing version and upgrade version
      # Existing version will come from confstore.
      # Upgrade version will come from release.info file or rpm.
      lookup_service = { "haproxy"      : [],
                         "s3server"     : ["keystore", "s3"],
                         "authserver"   : ["keystore", "auth"],
                         "s3bgschedular": ["bgdelete"],
                         "s3bgworker"   : ["bgdelete"]
      }
      g_upgrade_items = {
        's3' : {
              'configFile' : os.path.join(self.base_config_file_path, "s3/conf/s3config.yaml"),
              'oldSampleFile' : os.path.join(self.s3_tmp_dir, "s3config.yaml.sample.old"),
              'newSampleFile' : os.path.join(self.base_config_file_path, "s3/conf/s3config.yaml.sample"),
              'unsafeAttributesFile' : os.path.join(self.base_config_file_path, "s3/conf/s3config_unsafe_attributes.yaml"),
              'fileType' : 'yaml://'
         },
         'auth' : {
              'configFile' : os.path.join(self.base_config_file_path, "auth/resources/authserver.properties"),
              'oldSampleFile' : os.path.join(self.s3_tmp_dir, "authserver.properties.sample.old"),
              'newSampleFile' : os.path.join(self.base_config_file_path, "auth/resources/authserver.properties.sample"),
              'unsafeAttributesFile' : os.path.join(self.base_config_file_path, "auth/resources/authserver_unsafe_attributes.properties"),
              'fileType' : 'properties://'
         },
         'keystore' : {
              'configFile' : os.path.join(self.base_config_file_path, "auth/resources/keystore.properties"),
              'oldSampleFile' : os.path.join(self.s3_tmp_dir,"keystore.properties.sample.old"),
              'newSampleFile' : os.path.join(self.base_config_file_path, "auth/resources/keystore.properties.sample"),
              'unsafeAttributesFile' : os.path.join(self.base_config_file_path, "auth/resources/keystore_unsafe_attributes.properties"),
              'fileType' : 'properties://'
         },
         'bgdelete' : {
              'configFile' : os.path.join(self.base_config_file_path, "s3/s3backgrounddelete/config.yaml"),
              'oldSampleFile' : os.path.join(self.s3_tmp_dir, "config.yaml.sample.old"),
              'newSampleFile' : os.path.join(self.base_config_file_path, "s3/s3backgrounddelete/config.yaml.sample"),
              'unsafeAttributesFile' : os.path.join(self.base_config_file_path, "s3/s3backgrounddelete/s3backgrounddelete_unsafe_attributes.yaml"),
              'fileType' : 'yaml://'
         },
         'cluster' : {
              'configFile' : os.path.join(self.base_config_file_path, "s3/s3backgrounddelete/s3_cluster.yaml"),
              'oldSampleFile' : os.path.join(self.s3_tmp_dir, "s3_cluster.yaml.sample.old"),
              'newSampleFile' : os.path.join(self.base_config_file_path, "s3/s3backgrounddelete/s3_cluster.yaml.sample"),
              'unsafeAttributesFile' : os.path.join(self.base_config_file_path, "s3/s3backgrounddelete/s3_cluster_unsafe_attributes.yaml"),
              'fileType' : 'yaml://'
         }
      }
      if self.services is None:
        self.services = "haproxy,s3server,authserver,s3bgschedular,s3bgworker"
      service_list = self.services.split(",")
      for ser_key in service_list:
        if ser_key not in lookup_service:
          raise Exception(f'ERROR: {ser_key} incorrect service passed')

      self.logger.info("validations started")
      self.phase_prereqs_validate(self.name)
      self.logger.info("validations completed")

      # after rpm install, sample and unsafe attribute files needs to copy to /etc/cortx for merge logic
      self.logger.info("Copy .ample and unsafe attribute files started")
      self.copy_config_files()
      self.logger.info("Copy .ample and unsafe attribute files completed")

      # service based check for old files and copy necessary config files
      self.logger.info("Check and copy files based on service - start")
      old_file_list = []
      for ser_key in service_list:
        ser_list = lookup_service[ser_key]
        for upgrade_item in ser_list:
          old_file_list.append(g_upgrade_items[upgrade_item]['oldSampleFile'])
        self.old_file_check(old_file_list)
      self.logger.info("Check and copy files based on service - complete")

      # merge_configs() is imported from the merge.py
      # Upgrade config files
      self.logger.info("merge configs started")
      file_list = []
      for ser_key in service_list:
        ser_list = lookup_service[ser_key]
        for upgrade_item in ser_list:
          file_list.append(g_upgrade_items[upgrade_item]['configFile'],
                           g_upgrade_items[upgrade_item]['oldSampleFile'],
                           g_upgrade_items[upgrade_item]['newSampleFile'],
                           g_upgrade_items[upgrade_item]['unsafeAttributesFile'],
                           g_upgrade_items[upgrade_item]['fileType'])
          merge_configs(self.base_config_file_path, self.s3_tmp_dir, file_list)
      self.logger.info("merge configs completed")

      # Validating config files after upgrade
      self.logger.info("config file validations started")
      self.validate_config_files(self.name)
      self.logger.info("config file validations completed")

      # post merge copy call
      self.logger.info("post merge copy files start")
      self.post_merge_copy()
      self.logger.info("post merge copy files complete")

      # Remove temporary .old files from S3 temporary location
      self.logger.info("Remove sample.old files started")
      regex = "*.sample.old"
      self.DeleteFileOrDirWithRegex(self.s3_tmp_dir, regex)
      self.logger.info("Remove sample.old files completed")

    except Exception as e:
      raise S3PROVError(f'process: {self.name} failed with exception: {e}')

  def copy_config_files(self):
    """Copy sample and unsafe attribute config files from /opt/seagate/cortx to /etc/cortx."""
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
      dest_config_file = config_file.replace("/opt/seagate/cortx", "/etc/cortx")
      self.logger.info(f"Dest config file: {dest_config_file}")
      os.makedirs(os.path.dirname(dest_config_file), exist_ok=True)
      shutil.move(config_file, dest_config_file)
      self.logger.info("Config file copied successfully to /etc/cortx")

  def old_file_check(self, old_file_list):
    """Check for '.old' files and fail if not found."""
    for sample_old_file in old_file_list:
        if not os.path.exists(sample_old_file):
          self.logger.info(f"{sample_old_file} backup file not found")
          raise S3PROVError(f'process: {self.name} failed')

  def post_merge_copy(self):
    """Post merge copy."""
    # Copy /opt/seagate/cortx/s3/conf/s3config.yaml.sample to
    #      /etc/cortx/s3/tmp/s3config.yaml.sample.old
    s3_config_sample_file = self.get_confkey('S3_CONFIG_SAMPLE_FILE')
    s3_config_sample_file_old = os.path.join(self.s3_tmp_dir, "s3config.yaml.sample.old")
    shutil.copy(s3_config_sample_file, s3_config_sample_file_old)
    # Copy /opt/seagate/cortx/auth/resources/authserver.properties.sample to
    #      /etc/cortx/s3/tmp/authserver.properties.sample.old
    s3_auth_config_sample_file = self.get_confkey('S3_AUTHSERVER_CONFIG_SAMPLE_FILE')
    s3_auth_config_sample_file_old = os.path.join(self.s3_tmp_dir, "authserver.properties.sample.old")
    shutil.copy(s3_auth_config_sample_file, s3_auth_config_sample_file_old)
    # Copy /opt/seagate/cortx/auth/resources/keystore.properties.sample to
    #      /etc/cortx/s3/tmp/keystore.properties.sample.old
    s3_keystore_config_sample_file = self.get_confkey('S3_KEYSTORE_CONFIG_SAMPLE_FILE')
    s3_keystore_config_sample_file_old = os.path.join(self.s3_tmp_dir, "keystore.properties.sample.old")
    shutil.copy(s3_keystore_config_sample_file, s3_keystore_config_sample_file_old)
    # Copy /opt/seagate/cortx/s3/s3backgrounddelete/config.yaml.sample to
    #      /etc/cortx/s3/tmp/config.yaml.sample.old
    s3_bgdelete_config_sample_file = self.get_confkey('S3_BGDELETE_CONFIG_SAMPLE_FILE')
    s3_bgdelete_config_sample_file_old = os.path.join(self.s3_tmp_dir, "config.yaml.sample.old")
    shutil.copy(s3_bgdelete_config_sample_file, s3_bgdelete_config_sample_file_old)
    # Copy /opt/seagate/cortx/s3/s3backgrounddelete/s3_cluster.yaml.sample to
    #      /etc/cortx/s3/tmp/s3_cluster.yaml.sample.old
    s3_cluster_config_sample_file = self.get_confkey('S3_CLUSTER_CONFIG_SAMPLE_FILE')
    s3_cluster_config_sample_file_old = os.path.join(self.s3_tmp_dir, "s3_cluster.yaml.sample.old")
    shutil.copy(s3_cluster_config_sample_file, s3_cluster_config_sample_file_old)
    self.logger.info("copying config files for upgrade complete")

