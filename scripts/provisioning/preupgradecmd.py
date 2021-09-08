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

import os
import ntpath
from shutil import copyfile
from pathlib import Path
from setupcmd import SetupCmd, S3PROVError

class PreUpgradeCmd(SetupCmd):
  """Pre Upgrade Setup Cmd."""
  name = "preupgrade"

  def __init__(self, config: str, module: str = None):
    """Constructor."""
    try:
      super(PreUpgradeCmd, self).__init__(config, module)
    except Exception as e:
      raise e

  def process(self):
    """Main processing function."""
    self.logger.info(f"Processing phase = {self.name}, config = {self.url}, module = {self.module}")
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
    """function to backup .sample config file to .old"""
    sampleconfigfiles = {
      self.get_confkey('S3_CONFIG_SAMPLE_FILE').replace("/opt/seagate/cortx", "/etc/cortx"),
      self.get_confkey('S3_AUTHSERVER_CONFIG_SAMPLE_FILE').replace("/opt/seagate/cortx", "/etc/cortx"),
      self.get_confkey('S3_KEYSTORE_CONFIG_SAMPLE_FILE').replace("/opt/seagate/cortx", "/etc/cortx"),
      self.get_confkey('S3_BGDELETE_CONFIG_SAMPLE_FILE').replace("/opt/seagate/cortx", "/etc/cortx"),
      self.get_confkey('S3_CLUSTER_CONFIG_SAMPLE_FILE').replace("/opt/seagate/cortx", "/etc/cortx")
      }

    # make S3 temp dir if does not exist 
    Path(self.s3_tmp_dir).mkdir(parents=True, exist_ok=True)

    for sampleconfigfile in sampleconfigfiles:
      # check file exist
      if os.path.isfile(sampleconfigfile):
        #backup .sample to .old at S3 temporary location
        copyfile(sampleconfigfile, os.path.join(self.s3_tmp_dir, ntpath.basename(sampleconfigfile) + ".old"))
        self.logger.info(f"sample config file {sampleconfigfile} backup successfully")
      else:
         self.logger.error(f"sample config file {sampleconfigfile} does not exist")
         raise Exception(f"sample config file {sampleconfigfile} does not exist")
