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

from json.decoder import JSONDecodeError
import sys
import json
import os

from cortx.utils.validator.v_pkg import PkgV
from cortx.utils.validator.v_service import ServiceV
from cortx.utils.validator.v_path import PathV

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
    sys.stdout.write(f"Processing {self.name}\n")
    try:
      with open(self._preqs_conf_file) as preqs_conf:
        preqs_conf_json = json.load(preqs_conf)
      self.validate_pre_requisites(rpms=preqs_conf_json['rpms'],
                              services=preqs_conf_json['services'],
                              pip3s=preqs_conf_json['pip3s'],
                              files=preqs_conf_json['exists']
                            )
    except IOError as e:
      print(f'{self._preqs_conf_file} open failed, error: {e}')
      raise e
    except JSONDecodeError as e:
      print(f'fail to decode json file:{self._preqs_conf_file}, error: {e}')
      raise e
    except Exception as e:
      # exception in validation
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
