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
import shutil
from os import path
from s3confstore.cortx_s3_confstore import S3CortxConfStore
from s3cipher.cortx_s3_cipher import CortxS3Cipher
from cortx.utils.validator.v_pkg import PkgV
from cortx.utils.validator.v_service import ServiceV
from cortx.utils.validator.v_path import PathV
from cortx.utils.process import SimpleProcess

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
  ldap_mdb_folder = "/var/lib/ldap"
  s3_prov_config = "/opt/seagate/cortx/s3/mini-prov/s3_prov_config.yaml"
  _preqs_conf_file = "/opt/seagate/cortx/s3/mini-prov/s3setup_prereqs.json"
  #TODO
  # add the service name and HA service name in the following dictionary
  # as key value pair after confirming from the HA team
  # for e.g.
  # ha_service_map = {'haproxy': 'haproxy',
  #                's3backgroundproducer': 's3backprod',
  #                's3backgroundconsumer': 's3backcons',
  #                's3server@*': 's3server-*',
  #                's3authserver': 's3auth'}
  ha_service_map = {}

  def __init__(self, config: str):
    """Constructor."""
    if config is None:
      return

    if not config.strip():
      sys.stderr.write(f'config url:[{config}] must be a valid url path\n')
      raise Exception('empty config URL path')

    self.endpoint = None
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

  def read_endpoint_value(self):
    if self.endpoint is None:
      self.endpoint = self.get_confvalue(self.get_confkey('CONFSTORE_ENDPOINT_KEY'))

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

      if encrypted_ldapadmin_pass != None:
        self.ldap_passwd = s3cipher_obj.decrypt(cipher_key, encrypted_ldapadmin_pass)

      if encrypted_rootdn_pass != None:
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

  def shutdown_services(self, s3services_list):
    """Stop services."""
    for service_name in s3services_list:
      try:
        # if service name not found in the ha_service_map then use systemctl
        service_name = self.ha_service_map[service_name]
        cmd = ['cortx', 'stop',  f'{service_name}']
      except KeyError:
        cmd = ['/bin/systemctl', 'stop',  f'{service_name}']
      handler = SimpleProcess(cmd)
      sys.stdout.write(f"shutting down {service_name}\n")
      res_op, res_err, res_rc = handler.run()
      if res_rc != 0:
        raise Exception(f"{cmd} failed with err: {res_err}, out: {res_op}, ret: {res_rc}")

  def start_services(self, s3services_list):
    """Start services specified as parameter."""
    for service_name in s3services_list:
      try:
        # if service name not found in the ha_service_map then use systemctl
        service_name = self.ha_service_map[service_name]
        cmd = ['cortx', 'start',  f'{service_name}']
      except KeyError:
        cmd = ['/bin/systemctl', 'start',  f'{service_name}']
      handler = SimpleProcess(cmd)
      sys.stdout.write(f"starting {service_name}\n")
      res_op, res_err, res_rc = handler.run()
      if res_rc != 0:
        raise Exception(f"{cmd} failed with err: {res_err}, out: {res_op}, ret: {res_rc}")

  def restart_services(self, s3services_list):
    """Restart services specified as parameter."""
    for service_name in s3services_list:
      try:
        # if service name not found in the ha_service_map then use systemctl
        service_name = self.ha_service_map[service_name]
        cmd = ['cortx', 'restart',  f'{service_name}']
      except KeyError:
        cmd = ['/bin/systemctl', 'restart',  f'{service_name}']
      handler = SimpleProcess(cmd)
      sys.stdout.write(f"restarting {service_name}\n")
      res_op, res_err, res_rc = handler.run()
      if res_rc != 0:
        raise Exception(f"{cmd} failed with err: {res_err}, out: {res_op}, ret: {res_rc}")

  def delete_mdb_files(self):
    """Deletes ldap mdb files."""
    for files in os.listdir(self.ldap_mdb_folder):
      path = os.path.join(self.ldap_mdb_folder,files)
      if os.path.isfile(path) or os.path.islink(path):
        os.unlink(path)
      elif os.path.isdir(path):
        shutil.rmtree(path)
