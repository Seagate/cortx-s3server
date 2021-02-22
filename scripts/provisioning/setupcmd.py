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

from s3confstore.cortx_s3_confstore import S3CortxConfStore
from s3cipher import CortxS3Cipher

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
    try:
      localconfstore = S3CortxConfStore(f'yaml://{self.s3_prov_config}', 's3provindex')

      s3cipher_obj = CortxS3Cipher(None, False, 0, localconfstore.get_config('CONFSTORE_OPENLDAP_CONST_KEY'))
      cipher_key = s3cipher_obj.generate_key()

      encrypted_ldapadmin_pass = self.s3confstore.get_config(localconfstore.get_config('CONFSTORE_LDAPADMIN_PASSWD_KEY'))
      self.ldap_passwd = s3cipher_obj.decrypt(cipher_key, encrypted_ldapadmin_pass)

      self.ldap_user = self.s3confstore.get_config(localconfstore.get_config('CONFSTORE_LDAPADMIN_USER_KEY'))
    except Exception as e:
      sys.stderr.write(f'read ldap credentials failed, error: {e}\n')
      raise e
