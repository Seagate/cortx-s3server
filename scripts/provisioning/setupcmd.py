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
from os import path
from cortx.utils.conf_store import Conf
from s3cipher.cortx_s3_cipher import CortxS3Cipher
from cortx.utils.validator.v_pkg import PkgV
from cortx.utils.validator.v_service import ServiceV
from cortx.utils.validator.v_path import PathV

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
  _preqs_conf_file = "/opt/seagate/cortx/s3/mini-prov/s3setup_prereqs.json"

  def __init__(self, config: str):
    """Constructor."""
    if config is None:
      return

    if not config.strip():
      sys.stderr.write(f'config url:[{config}] must be a valid url path\n')
      raise Exception('empty config URL path')

    self._url = config

    Conf.load('localstore', f'yaml://{self.s3_prov_config}')
    Conf.load('provstore', self._url)

    # machine_id will be used to read confstore keys
    with open('/etc/machine-id') as f:
      self.machine_id = f.read().strip()

    self.cluster_id = Conf.get('provstore',
                              Conf.get('localstore',
                              'CONFSTORE_CLUSTER_ID_KEY').format(self.machine_id))

  @property
  def url(self) -> str:
    return self._url

  def read_ldap_credentials(self):
    """Get 'ldapadmin' user name and password from confstore."""
    try:
      s3cipher_obj = CortxS3Cipher(None, False, 0, Conf.get('localstore', 'CONFSTORE_OPENLDAP_CONST_KEY'))
      cipher_key = s3cipher_obj.generate_key()

      self.ldap_user = Conf.get('provstore', Conf.get('localstore', 'CONFSTORE_LDAPADMIN_USER_KEY'))

      encrypted_ldapadmin_pass = Conf.get('provstore', Conf.get('localstore', 'CONFSTORE_LDAPADMIN_PASSWD_KEY'))
      encrypted_rootdn_pass = Conf.get('provstore', Conf.get('localstore', 'CONFSTORE_ROOTDN_PASSWD_KEY'))

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

        Conf.load('write_cluster_id_idx', f'yaml://{op_file}')
        Conf.set('write_cluster_id_idx', f'{key}', f'{self.cluster_id}')
        Conf.save('write_cluster_id_idx')

        updated_cluster_id = Conf.get('write_cluster_id_idx', f'{key}')

        if updated_cluster_id != self.cluster_id:
          raise S3PROVError(f'set_config failed to set {key}: {self.cluster_id} in {op_file} \n')
    except Exception as e:
      raise S3PROVError(f'exception: {e}\n')

  def validate_pre_requisites(self,
                        rpms: list = None,
                        pip3s: list = None,
                        services: list = None,
                        files: list = None):
    """Validate pre requisites using cortx-py-utils validator."""
    sys.stdout.write(f'Validations running from {self._preqs_conf_file}\n')
    if pip3s:
      PkgV().validate('pip3s', pip3s)
    if services:
      ServiceV().validate('isrunning', services)
    if rpms:
      PkgV().validate('rpms', rpms)
    if files:
      PathV().validate('exists', files)

  def phase_prereqs_validate(self, phase_name: str):
    """Validate pre requisites using cortx-py-utils validator for the 'phase_name'."""
    if not os.path.isfile(self._preqs_conf_file):
      raise FileNotFoundError(f'pre-requisite json file: {self._preqs_conf_file} not found')
    _prereqs_confstore = S3CortxConfStore(f'json://{self._preqs_conf_file}', f'{phase_name}')
    try:
      prereqs_block = _prereqs_confstore.get_config(f'{phase_name}')
      if prereqs_block is not None:
        self.validate_pre_requisites(rpms=_prereqs_confstore.get_config(f'{phase_name}>rpms'),
                                services=_prereqs_confstore.get_config(f'{phase_name}>services'),
                                pip3s=_prereqs_confstore.get_config(f'{phase_name}>pip3s'),
                                files=_prereqs_confstore.get_config(f'{phase_name}>files'))
    except Exception as e:
      raise S3PROVError(f'ERROR: {phase_name} prereqs validations failed, exception: {e} \n')
