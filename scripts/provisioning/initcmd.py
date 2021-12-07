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

class InitCmd(SetupCmd):
  """Init Setup Cmd."""
  name = "init"

  def __init__(self, config: str, services: str = None):
    """Constructor."""
    try:
      super(InitCmd, self).__init__(config, services)
      self.setup_type = self.get_confvalue_with_defaults('CONFIG>CONFSTORE_SETUP_TYPE')
      self.logger.info(f'log file path : {self.setup_type}')
      self.cluster_id = self.get_confvalue_with_defaults('CONFIG>CONFSTORE_CLUSTER_ID_KEY')
      self.logger.info(f'Cluster	id : {self.cluster_id}')
      self.base_config_file_path = self.get_confvalue_with_defaults('CONFIG>CONFSTORE_BASE_CONFIG_PATH')
      self.logger.info(f'config file path : {self.base_config_file_path}')
      self.base_log_file_path = self.get_confvalue_with_defaults('CONFIG>CONFSTORE_BASE_LOG_PATH')
      self.logger.info(f'log file path : {self.base_log_file_path}')

    except Exception as e:
      raise e

  def process(self):
    """Main processing function."""
    self.logger.info(f"Processing phase = {self.name}, config = {self.url}, service = {self.services}")
    self.logger.info("validations started")
    self.phase_prereqs_validate(self.name)
    self.phase_keys_validate(self.url, self.name)


    #s3server
    self.validate_config_file(self.get_confkey('S3_CONFIG_FILE').replace("/opt/seagate/cortx", self.base_config_file_path),
                                self.get_confkey('S3_CONFIG_SAMPLE_FILE').replace("/opt/seagate/cortx", self.base_config_file_path),
                                'yaml://')
	#authserver
    self.validate_config_file(self.get_confkey('S3_AUTHSERVER_CONFIG_FILE').replace("/opt/seagate/cortx", self.base_config_file_path),
                              self.get_confkey('S3_AUTHSERVER_CONFIG_SAMPLE_FILE').replace("/opt/seagate/cortx", self.base_config_file_path),
                              'properties://')
    self.validate_config_file(self.get_confkey('S3_KEYSTORE_CONFIG_FILE').replace("/opt/seagate/cortx", self.base_config_file_path),
                              self.get_confkey('S3_KEYSTORE_CONFIG_SAMPLE_FILE').replace("/opt/seagate/cortx", self.base_config_file_path),
                              'properties://')
    #s3bgschedular
    self.validate_config_file(self.get_confkey('S3_BGDELETE_CONFIG_FILE').replace("/opt/seagate/cortx", self.base_config_file_path),
                              self.get_confkey('S3_BGDELETE_CONFIG_SAMPLE_FILE').replace("/opt/seagate/cortx", self.base_config_file_path),
                              'yaml://')
    #s3bgworker
    self.validate_config_file(self.get_confkey('S3_BGDELETE_CONFIG_FILE').replace("/opt/seagate/cortx", self.base_config_file_path),
                              self.get_confkey('S3_BGDELETE_CONFIG_SAMPLE_FILE').replace("/opt/seagate/cortx", self.base_config_file_path),
                              'yaml://')


    self.logger.info("validations completed")
