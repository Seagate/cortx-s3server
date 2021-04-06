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
  ldap_root_user = None
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
      'CONFIG>CONFSTORE_CLUSTER_ID_KEY').replace("machine-id", self.machine_id))

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
      self.endpoint = self.get_confvalue(self.get_confkey('TEST>CONFSTORE_ENDPOINT_KEY'))

  def read_ldap_credentials(self):
    """Get 'ldapadmin' user name and password from confstore."""
    try:
      s3cipher_obj = CortxS3Cipher(None,
                                False,
                                0,
                                self.get_confkey('CONFSTORE_OPENLDAP_CONST_KEY'))

      cipher_key = s3cipher_obj.generate_key()

      self.ldap_user = self.get_confvalue(self.get_confkey('CONFIG>CONFSTORE_LDAPADMIN_USER_KEY'))

      encrypted_ldapadmin_pass = self.get_confvalue(self.get_confkey('CONFIG>CONFSTORE_LDAPADMIN_PASSWD_KEY'))

      self.ldap_root_user = self.get_confvalue(self.get_confkey('CONFIG>CONFSTORE_ROOTDN_USER_KEY'))

      encrypted_rootdn_pass = self.get_confvalue(self.get_confkey('CONFIG>CONFSTORE_ROOTDN_PASSWD_KEY'))

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

  def phase_keys_validate(self, arg_file: str, phase_name: str):
    """Validate keys of each phase derived from s3_prov_config and compare with argument file."""
    #token_list = ["machine-id", "cluster-id", "N"]
    phase_name = phase_name.upper()
    argument_file_confstore = S3CortxConfStore(arg_file, 'argument_file_index')
    try:
      # Get all keys from URL file
      arg_keys_list = argument_file_confstore.get_all_keys()

      # Since get_all_keys misses out listing entries inside
      # an array, the below code is required to fetch such
      # array entries. The result will be stored in yardstick
      # list which will be complete and will be used to verify
      # keys required for each phase.
      full_arg_keys_list = []
      for key in arg_keys_list:
        if ((key.find('[') != -1) and (key.find(']') != -1)):
          storage_set = self.get_confvalue(key)
          for set_key in storage_set:
            key = key + ">" + set_key
          storage_set = self.get_confvalue(key)
        full_arg_keys_list.append(key)

      # The s3 prov config file has below pairs :
      # "Key Constant" : "Actual Key"

      # Example of "Key Constant" :
      #   CONFSTORE_SITE_COUNT_KEY
      #   PREPARE
      #   CONFIG>CONFSTORE_LDAPADMIN_USER_KEY
      #   INIT

      # Example of "Actual Key" :
      #   cluster>{}>site_count
      #   cortx>software>openldap>sgiam>user

      # When we try to run get_all_keys on this file,
      # it returns all the "Key Constant" from the
      # config file, which may have PHASE as the
      # root attribute. To get "Actual Key", we need
      # to run get_confkey on each "Key Constant" and
      # format it based on PHASE.

      # Note that for each of these "Key Constant",
      # there may not exist an "Actual Key" because
      # some phases do not have any "Actual Key".
      # Example of such cases -
      #   PREPARE
      #   INIT
      # For such examples, we skip that "Key Constant"
      #  and continue.

      prov_keys_list = self._s3_confkeys_store.get_all_keys()

      # Below hard-coded key is used to set the value
      # of 'storage_set_count'. This will be used to
      # set the array size of 'storage_set' if it is
      # found in the key string.
      site_count_key = "cluster>cluster-id>site>storage_set_count"
      if not self.cluster_id is None:
        site_count_key = site_count_key.replace("cluster-id", self.cluster_id)
      site_count_str = self.get_confvalue(site_count_key)
      if not site_count_str is None:
        site_count_n = int(site_count_str)
      else:
        site_count_n = 0
      # We have all "Key Constant" in prov_keys_list,
      # now extract "Actual Key" and format it to fill
      # regular expressions which will be used for below
      # for pattern matching. Also, in order to implement
      # hierarchy set the flags as below.
      phase_keys_list = []
      prev_phase = True
      curr_phase = False
      next_phase = False
      for key in prov_keys_list:
        # If PHASE is not relevant, skip the key.
        # Or set flag as appropriate. For test,
        # reset and cleanup, do not inherit keys
        # from previous phases.
        if next_phase:
          break
        if key.find(phase_name) == 0:
          prev_phase = False
          curr_phase = True
        else:
          if (
               phase_name == "TEST" or
               phase_name == "RESET" or
               phase_name == "CLEANUP"
             ):
              continue
          if not prev_phase:
            curr_phase = False
            next_phase = True
            break
        value = self.get_confkey(key)
        # If value does not exist which can be the
        # case for certain phases as mentioned above,
        # skip the value.
        if value is None:
          continue
        # If a value is found which needs to be filled
        # up with actual details, like machine-id or
        # cluster-id, replace it and create new value.
        # Also, fill values starting from zero upto
        # one less than array size extracted above
        # and make an array of format values if needed.
        # Then insert the formatted value in newly created
        # phase_keys_list which will be used to match against
        # full_arg_keys_list for validating each key.
        # A list of tokens is maintained above for reference.
        formatvalue = value
        formatvalue = formatvalue.replace("machine-id", self.machine_id)
        if not self.cluster_id is None:
          formatvalue = formatvalue.replace("cluster-id", self.cluster_id)
        if not formatvalue.find("N") == -1:
          formatvalue_n = formatvalue
          for i in range(site_count_n):
            formatvalue_n = formatvalue.replace("N", str(i))
            phase_keys_list.append(formatvalue_n)
        else:
          phase_keys_list.append(formatvalue)

      # Check whether each key in phase_keys_list
      # has matching pattern in full_arg_keys_list, if
      # no match found, raise exception.
      for phase_key in phase_keys_list:
        match_found = False
        for arg_key in full_arg_keys_list:
          if phase_key == arg_key and match_found is False:
            match_found = True
        if match_found is False:
          raise Exception(f'No match found for {phase_key}')

    except Exception as e:
      raise Exception(f'ERROR : Validating keys failed, exception {e}\n')

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

  def reload_services(self, s3services_list):
    """Reload services specified as parameter."""
    for service_name in s3services_list:
      try:
        # if service name not found in the ha_service_map then use systemctl
        service_name = self.ha_service_map[service_name]
        cmd = ['cortx', 'reload',  f'{service_name}']
      except KeyError:
        cmd = ['/bin/systemctl', 'reload',  f'{service_name}']
      handler = SimpleProcess(cmd)
      sys.stdout.write(f"reloading {service_name}\n")
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
