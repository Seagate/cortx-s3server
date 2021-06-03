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
from os import path
from s3confstore.cortx_s3_confstore import S3CortxConfStore
from setupcmd import SetupCmd, S3PROVError

class PostInstallCmd(SetupCmd):
  """PostInstall Setup Cmd."""
  name = "post_install"

  def __init__(self, config: str = None):
    """Constructor."""
    try:
      super(PostInstallCmd, self).__init__(config)
    except Exception as e:
      raise S3PROVError(f'exception: {e}')

  def process(self):
    """Main processing function."""
    self.logger.info(f"Processing {self.name} {self.url}")
    try:
      self.logger.info("validations started")
      self.phase_prereqs_validate(self.name)
      self.phase_keys_validate(self.url, self.name)
      self.validate_config_files(self.name)
      self.logger.info("validations completed")

      self.logger.info("update motr max units per request started")
      self.update_motr_max_units_per_request()
      self.logger.info("update motr max units per request completed")
    except Exception as e:
      raise S3PROVError(f'process: {self.name} failed with exception: {e}')

  def update_motr_max_units_per_request(self):
    """
    update S3_MOTR_MAX_UNITS_PER_REQUEST in the s3config file based on VM/OVA/HW
    S3_MOTR_MAX_UNITS_PER_REQUEST = 8 for VM/OVA
    S3_MOTR_MAX_UNITS_PER_REQUEST = 32 for HW
    """

    # get the motr_max_units_per_request count from the config file
    motr_max_units_per_request = self.get_confvalue(self.get_confkey('POST_INSTALL>CONFSTORE_S3_MOTR_MAX_UNITS_PER_REQUEST'))
    self.logger.info(f'motr_max_units_per_request: {motr_max_units_per_request}')

    # update the S3_MOTR_MAX_UNITS_PER_REQUEST in s3config.yaml file
    s3configfile = self.get_confkey('S3_CONFIG_FILE')
    if path.isfile(f'{s3configfile}') == False:
      self.logger.error(f'{s3configfile} file is not present')
      raise S3PROVError(f'{s3configfile} file is not present')
    else:
      motr_max_units_per_request_key = 'S3_MOTR_CONFIG>S3_MOTR_MAX_UNITS_PER_REQUEST'
      s3configfileconfstore = S3CortxConfStore(f'yaml://{s3configfile}', 'write_s3_motr_max_unit_idx')
      s3configfileconfstore.set_config(f'{motr_max_units_per_request_key}', f'{motr_max_units_per_request}', True)
      self.logger.info(f'Key {motr_max_units_per_request_key} updated successfully in {s3configfile}')
