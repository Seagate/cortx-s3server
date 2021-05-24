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
from cortx.utils.process import SimpleProcess

SANITY_SCRIPT_PATH = '/opt/seagate/cortx/s3/scripts/s3-sanity-test.sh'
LDAP_SWITCH = '-p'
ENDPOINT_SWITCH = '-e'

class TestCmd(SetupCmd):
  """Test Setup Cmd."""
  name = "test"

  def __init__(self, config: str, test_plan: str):
    """Constructor."""
    try:
      super(TestCmd, self).__init__(config)
      self.read_ldap_credentials()
      self.test_plan = test_plan

    except Exception as e:
      raise e

  def process(self):
    """Main processing function."""
    #TODO: remove the return in next sprint
    self.logger.info(f"Processing {self.name} {self.url}")
    self.logger.info("validations started")
    self.phase_prereqs_validate(self.name)
    self.phase_keys_validate(self.url, self.name)
    self.logger.info("validations completed")

    self.logger.info("test started")
    try:
      self.read_endpoint_value()
      self.logger.info(f"Endpoint fqdn {self.endpoint}")
      cmd = [SANITY_SCRIPT_PATH, LDAP_SWITCH,  f'{self.ldap_passwd}', ENDPOINT_SWITCH, f'{self.endpoint}']
      handler = SimpleProcess(cmd)
      stdout, stderr, retcode = handler.run()
      self.logger.info(f'output of s3-sanity-test.sh: {stdout}')
      if retcode != 0:
        self.logger.error(f'error of s3-sanity-test.sh: {stderr}')
        raise Exception(f"{cmd} failed with err: {stderr}, out: {stdout}, ret: {retcode}")
      else:
        self.logger.warning(f'warning of s3-sanity-test.sh: {stderr}')
    except Exception as e:
      raise Exception(f"{self}: {cmd} exception: {e}")
    self.logger.info("test completed")
