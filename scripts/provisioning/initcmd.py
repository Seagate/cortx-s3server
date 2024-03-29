#!/usr/bin/env python3
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

import sys

from setupcmd import SetupCmd
from cortx.utils.log import Log

class InitCmd(SetupCmd):
  """Init Setup Cmd."""
  name = "init"

  def __init__(self, config: str, services: str = None):
    """Constructor."""
    try:
      super(InitCmd, self).__init__(config, services)
      self.setup_type = self.get_confvalue_with_defaults('CONFIG>CONFSTORE_SETUP_TYPE')
      Log.info(f'log file path : {self.setup_type}')
      self.cluster_id = self.get_confvalue_with_defaults('CONFIG>CONFSTORE_CLUSTER_ID_KEY')
      Log.info(f'Cluster	id : {self.cluster_id}')
      self.base_config_file_path = self.get_confvalue_with_defaults('CONFIG>CONFSTORE_BASE_CONFIG_PATH')
      Log.info(f'config file path : {self.base_config_file_path}')
      self.base_log_file_path = self.get_confvalue_with_defaults('CONFIG>CONFSTORE_BASE_LOG_PATH')
      Log.info(f'log file path : {self.base_log_file_path}')

    except Exception as e:
      raise e

  def process(self):
    """Main processing function."""
    Log.info(f"Processing phase = {self.name}, config = {self.url}, service = {self.services}")
    Log.info("validations started")
    self.phase_prereqs_validate(self.name)
    self.phase_keys_validate(self.url, self.name)
    # validate config files as per services.
    Log.info("validate s3 cluster config file started")
    self.validate_config_file(self.get_confkey('S3_CLUSTER_CONFIG_FILE').replace("/opt/seagate/cortx", self.base_config_file_path),
                              self.get_confkey('S3_CLUSTER_CONFIG_SAMPLE_FILE').replace("/opt/seagate/cortx", self.base_config_file_path),
                              'yaml://')
    Log.info("validate s3 cluster config file completed")

    if self.service_haproxy in self.services:
      pass

    if self.service_s3server in self.services:
      Log.info("validate s3 config file started")
      self.validate_config_file(self.get_confkey('S3_CONFIG_FILE').replace("/opt/seagate/cortx", self.base_config_file_path),
                                self.get_confkey('S3_CONFIG_SAMPLE_FILE').replace("/opt/seagate/cortx", self.base_config_file_path),
                                'yaml://')
      Log.info("validate s3 config files completed")

    if self.service_authserver in self.services:
      Log.info("validate auth config file started")
      self.validate_config_file(self.get_confkey('S3_AUTHSERVER_CONFIG_FILE').replace("/opt/seagate/cortx", self.base_config_file_path),
                              self.get_confkey('S3_AUTHSERVER_CONFIG_SAMPLE_FILE').replace("/opt/seagate/cortx", self.base_config_file_path),
                              'properties://')
      self.validate_config_file(self.get_confkey('S3_KEYSTORE_CONFIG_FILE').replace("/opt/seagate/cortx", self.base_config_file_path),
                              self.get_confkey('S3_KEYSTORE_CONFIG_SAMPLE_FILE').replace("/opt/seagate/cortx", self.base_config_file_path),
                              'properties://')
      Log.info("validate auth config files completed")

    if self.service_bgscheduler in self.services:
      Log.info("validate s3 bgdelete scheduler config file started")
      self.validate_config_file(self.get_confkey('S3_BGDELETE_CONFIG_FILE').replace("/opt/seagate/cortx", self.base_config_file_path),
                                self.get_confkey('S3_BGDELETE_CONFIG_SAMPLE_FILE').replace("/opt/seagate/cortx", self.base_config_file_path),
                                'yaml://')
    Log.info("validate s3 bgdelete scheduler config files completed")

    if self.service_bgworker in self.services:
      Log.info("validate s3 bgdelete worker config file started")
      self.validate_config_file(self.get_confkey('S3_BGDELETE_CONFIG_FILE').replace("/opt/seagate/cortx", self.base_config_file_path),
                                self.get_confkey('S3_BGDELETE_CONFIG_SAMPLE_FILE').replace("/opt/seagate/cortx", self.base_config_file_path),
                                'yaml://')
    Log.info("validate s3 bgdelete worker config files completed")

    Log.info("validations completed")
