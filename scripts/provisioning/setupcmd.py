#!/usr/bin/env python3
#
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
import yaml
from yaml.error import YAMLError

from s3confstore.cortx_s3_confstore import S3CortxConfStore
from s3cipher.cortx_s3_cipher import CortxS3Cipher

class SetupCmd(object):
  """Base class for setup commands."""
  ldap_user = None
  ldap_passwd = None
  s3_prov_config = "/opt/seagate/cortx/s3/mini-prov/s3_prov_config.yaml"

  def __init__(self, config: str):
    """Constructor."""
    if not config.strip():
      sys.stderr.write(f'config url:[{config}] must be a valid url path\n')
      raise Exception('empty config URL path')

    self._url = config
    self._s3confstore = S3CortxConfStore(self._url)

  @property
  def url(self) -> str:
    return self._url

  @property
  def s3confstore(self) -> str:
    return self._s3confstore

  def read_ldap_credentials(self):
    """Get 'ldapadmin' user name and password from confstore."""
    prov_config_yaml = None

    try:
      with open(self.s3_prov_config) as provconf:
        prov_config_yaml = yaml.safe_load(provconf)

      s3cipher_obj = CortxS3Cipher(None, False, 0, prov_config_yaml['CONFSTORE_OPENLDAP_CONST_KEY'])
      cipher_key = s3cipher_obj.generate_key()

      encrypted_ldapadmin_pass = self.s3confstore.get_config(prov_config_yaml['CONFSTORE_LDAPADMIN_PASSWD_KEY'])
      self.ldap_passwd = s3cipher_obj.decrypt(cipher_key, encrypted_ldapadmin_pass)

      self.ldap_user = self.s3confstore.get_config(prov_config_yaml['CONFSTORE_LDAPADMIN_USER_KEY'])
    except IOError as ioe:
      sys.stderr.write(f'failed to open config file: {self.s3_prov_config}, err: {ioe}\n')
      raise ioe
    except YAMLError as ye:
      sys.stderr.write(f'yaml load failed for config file: {self.s3_prov_config}, err: {ye}\n')
      raise ye
    except Exception as e:
      sys.stderr.write(f'unknown exception: {e}\n')
      raise e
