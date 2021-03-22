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
import os

from cortx.utils.validator.v_pkg import PkgV
from cortx.utils.validator.v_service import ServiceV
from cortx.utils.validator.v_path import PathV
from s3confstore.cortx_s3_confstore import S3CortxConfStore

class PostInstallCmd:
  """PostInstall Setup Cmd."""
  name = "post_install"
  _preqs_conf_file = "/opt/seagate/cortx/s3/mini-prov/s3setup_prereqs.json"

  def __init__(self):
    """Constructor."""
    if not os.path.isfile(self._preqs_conf_file):
      raise FileNotFoundError(f'pre-requisite json file: {self._preqs_conf_file} not found')

  def process(self):
    """Main processing function."""
    sys.stdout.write("Running validations..\n")
    try:
      localconfstore = S3CortxConfStore(f'json://{self._preqs_conf_file}', 'confindex')
      self.validate_pre_requisites(rpms=localconfstore.get_config('rpms'),
                              services=localconfstore.get_config('services'),
                              pip3s=localconfstore.get_config('pip3s'),
                              files=localconfstore.get_config('exists')
                            )
      key_list = self.extract_keys_from_template()
      self.validate_keys(key_list)
    except Exception as e:
      raise e

  def validate_pre_requisites(self,
                        rpms: list = None,
                        pip3s: list = None,
                        services: list = None,
                        files: list = None):
    """Validate pre requisites using cortx-py-utils validator."""
    try:
      if pip3s:
        PkgV().validate('pip3s', pip3s)
      if services:
        ServiceV().validate('isrunning', services)
      if rpms:
        PkgV().validate('rpms', rpms)
      if files:
        PathV().validate('exists', files)
    except Exception as e:
      sys.stderr.write('ERROR: post_install validations failed.\n')
      sys.stderr.write(f"{e}, config:{self._preqs_conf_file}\n")
      raise e

  def  extract_keys_from_template(self):
    """Extract all keys in template."""
    index = "postinstall_s3keys_index"
    filename = "/opt/seagate/cortx/s3/conf/s3.post_install.tmpl.1-node.sample"
    try:
      postinstallconfstorekeys = S3CortxConfStore(filename, index)
      return postinstallconfstorekeys.get_all_keys()
    except Exception as e:
      sys.stderr.write('ERROR: extracting keys failed.\n')
      raise e

  def validate_keys(self, keylist: list):
    """Check if each key of keylist is found in confstore."""
    postinstallconfstorekeys = S3CortxConfStore()
    for key in keylist:
      if postinstallconfstorekeys.get_config(key) is None:
        raise Exception("Key {} not found\n".format(key))
