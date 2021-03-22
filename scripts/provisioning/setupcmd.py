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
from s3cipher.cortx_s3_cipher import CortxS3Cipher

class S3PROVError(Exception):
  """Parent class for the s3 provisioner error classes."""
  pass

class SetupCmd(object):
  """Base class for setup commands."""
  ldap_user = None
  ldap_passwd = None
  rootdn_passwd = None
  cluster_id = None
  machine_id = None
  s3_prov_config = "/opt/seagate/cortx/s3/mini-prov/s3_prov_config.yaml"

  def __init__(self, config: str):
    """Constructor."""
    if not config.strip():
      sys.stderr.write(f'config url:[{config}] must be a valid url path\n')
      raise Exception('empty config URL path')

    self._url = config
    self._provisioner_confstore = S3CortxConfStore(self._url, 'setup_prov_index')
    self._s3_confkeys_store = S3CortxConfStore(f'yaml://{self.s3_prov_config}', 'setup_s3keys_index')

    # machine_id will be used to read confstore keys
    with open('/etc/machine-id') as f:
      self.machine_id = f.read().strip()

    self.cluster_id = self.get_confvalue(self.get_confkey(
      'CONFSTORE_CLUSTER_ID_KEY').format(self.machine_id))

  @property
  def url(self) -> str:
    return self._url

  @property
  def provisioner_confstore(self) -> str:
    return self._provisioner_confstore

  @property
  def s3_confkeys_store(self) -> str:
    return self._s3_confkeys_store

  def get_confkey(self, key: str):
    assert self.s3_confkeys_store != None
    return self.s3_confkeys_store.get_config(key)

  def get_confvalue(self, key: str):
    assert self.provisioner_confstore != None
    return self.provisioner_confstore.get_config(key)

  def read_ldap_credentials(self):
    """Get 'ldapadmin' user name and password from confstore."""
    try:
      s3cipher_obj = CortxS3Cipher(None,
                                False,
                                0,
                                self.get_confkey('CONFSTORE_OPENLDAP_CONST_KEY'))

      cipher_key = s3cipher_obj.generate_key()

      self.ldap_user = self.get_confvalue(self.get_confkey('CONFSTORE_LDAPADMIN_USER_KEY'))

      encrypted_ldapadmin_pass = self.get_confvalue(self.get_confkey('CONFSTORE_LDAPADMIN_PASSWD_KEY'))

      encrypted_rootdn_pass = self.get_confvalue(self.get_confkey('CONFSTORE_ROOTDN_PASSWD_KEY'))

      self.ldap_passwd = s3cipher_obj.decrypt(cipher_key, encrypted_ldapadmin_pass)
      self.rootdn_passwd = s3cipher_obj.decrypt(cipher_key, encrypted_rootdn_pass)

    except Exception as e:
      sys.stderr.write(f'read ldap credentials failed, error: {e}\n')
      raise e

  def update_cluster_id(self, op_file: str = "/opt/seagate/cortx/s3/s3backgrounddelete/s3_cluster.yaml"):
    """Set 'cluster_id' to op_file."""
    try:
      if path.isfile(f'{op_file}') == False:
        raise S3PROVError(f'{op_file} must be present\n')
      else:
        key = 'cluster_config>cluster_id'
        opfileconfstore = S3CortxConfStore(f'yaml://{op_file}', 'write_cluster_id_idx')
        opfileconfstore.set_config(f'{key}', f'{self.cluster_id}', True)
        updated_cluster_id = opfileconfstore.get_config(f'{key}')

        if updated_cluster_id != self.cluster_id:
          raise S3PROVError(f'set_config failed to set {key}: {self.cluster_id} in {op_file} \n')
    except Exception as e:
      raise S3PROVError(f'exception: {e}\n')
