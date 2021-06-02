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

import sys
import os
import ntpath
from shutil import copyfile
from setupcmd import SetupCmd, S3PROVError
from merge import merge_configs

class PreUpgradeCmd(SetupCmd):
  """Pre Upgrade Setup Cmd."""
  name = "preupgrade"

  def __init__(self, config: str):
    """Constructor."""
    try:
      super(PreUpgradeCmd, self).__init__(config)
    except Exception as e:
      raise e

  def process(self):
    """Main processing function."""
    self.logger.info(f"Processing {self.name} {self.url}")
    try:
      self.logger.info("validations started")
      self.phase_prereqs_validate(self.name)
      self.logger.info("validations completed")

      # Backing up .sample config file to .old
      self.logger.info("Backup .sample to .old started")
      self.backup_sample_file()
      self.logger.info("Backup .sample to .old completed")
    except Exception as e:
      raise S3PROVError(f'process: {self.name} failed with exception: {e}')

  def backup_sample_file(self):
    """ function to backup .sample config file to .old """
    sampleconfigfiles = {
      "/opt/seagate/cortx/s3/conf/s3config.yaml.sample",
      "/opt/seagate/cortx/s3/s3backgrounddelete/config.yaml.sample",
      "/opt/seagate/cortx/s3/s3backgrounddelete/s3_cluster.yaml.sample",
      "/opt/seagate/cortx/auth/resources/keystore.properties.sample",
      "/opt/seagate/cortx/auth/resources/authserver.properties.sample"
    }

    for sampleconfigfile in sampleconfigfiles:
      # check file exist
      if os.path.isfile(sampleconfigfile): 
        #backup .sample to .old at S3 temporary location
        copyfile(sampleconfigfile, os.path.join(self.s3_tmp_dir, ntpath.basename(sampleconfigfile) + ".old"))
        self.logger.info(f"sample config file {sampleconfigfile} backup successfully")
      else:
         self.logger.error(f"sample config file {sampleconfigfile} does not exist")
         raise Exception(f"sample config file {sampleconfigfile} does not exist")