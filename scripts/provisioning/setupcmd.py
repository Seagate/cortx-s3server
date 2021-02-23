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
  server_nodes_count = 0
  hosts_list = None
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
      localconfstore = S3CortxConfStore(f'yaml://{self.s3_prov_config}', 'read_ldap_credentialsidx')

      s3cipher_obj = CortxS3Cipher(None, False, 0, localconfstore.get_config('CONFSTORE_OPENLDAP_CONST_KEY'))
      cipher_key = s3cipher_obj.generate_key()

      encrypted_ldapadmin_pass = self.s3confstore.get_config(localconfstore.get_config('CONFSTORE_LDAPADMIN_PASSWD_KEY'))
      self.ldap_passwd = s3cipher_obj.decrypt(cipher_key, encrypted_ldapadmin_pass)

      self.ldap_user = self.s3confstore.get_config(localconfstore.get_config('CONFSTORE_LDAPADMIN_USER_KEY'))

      encrypted_rootdn_pass = self.s3confstore.get_config(localconfstore.get_config('CONFSTORE_ROOTDN_PASSWD_KEY'))
      self.rootdn_passwd = s3cipher_obj.decrypt(cipher_key, encrypted_rootdn_pass)

    except Exception as e:
      sys.stderr.write(f'read ldap credentials failed, error: {e}\n')
      raise e

  def read_cluster_id(self):
    """Get 'cluster>cluster_id' from confstore."""

    try:
      localconfstore = S3CortxConfStore(f'yaml://{self.s3_prov_config}', 'read_cluster_ididx')
      self.cluster_id = self.s3confstore.get_config(localconfstore.get_config('CONFSTORE_CLUSTER_ID_KEY'))
    except Exception as e:
      raise S3PROVError(f'exception: {e}\n')

  def read_node_info(self):
    """Call API get_nodecount from confstore."""
    try:
      self.server_nodes_count = self.s3confstore.get_nodecount()
      self.hosts_list = self.s3confstore.get_nodenames_list()
    except Exception as e:
      raise S3PROVError(f'unknown exception: {e}\n')
