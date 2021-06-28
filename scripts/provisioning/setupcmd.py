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
import re
import shutil
import glob
import socket
from os import path
from s3confstore.cortx_s3_confstore import S3CortxConfStore
from s3cipher.cortx_s3_cipher import CortxS3Cipher
from cortx.utils.validator.v_pkg import PkgV
from cortx.utils.validator.v_service import ServiceV
from cortx.utils.validator.v_path import PathV
from cortx.utils.validator.v_network import NetworkV
from cortx.utils.process import SimpleProcess
import logging

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
  s3_tmp_dir = "/opt/seagate/cortx/s3/tmp"
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

  def __init__(self,config: str):
    """Constructor."""
    self.endpoint = None
    self._url = None
    self._provisioner_confstore = None
    self._s3_confkeys_store = None
    self.machine_id = None
    self.cluster_id = None

    s3deployment_logger_name = "s3-deployment-logger-" + "[" + str(socket.gethostname()) + "]"
    self.logger = logging.getLogger(s3deployment_logger_name)
    
    if config is None:
      self.logger.warning(f'Empty Config url')
      return

    if not config.strip():
      self.logger.error(f'Config url:[{config}] must be a valid url path')
      raise Exception('Empty config URL path')

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

      if encrypted_ldapadmin_pass != None:
        self.ldap_passwd = s3cipher_obj.decrypt(cipher_key, encrypted_ldapadmin_pass)

      if self.ldap_user != "sgiamadmin":
        raise ValueError('provide correct rootdn username')      

    except Exception as e:
      self.logger.error(f'read ldap credentials failed, error: {e}')
      raise e

  def write_rootdn_credentials(self):
    """Set rootdn username and password to opfile."""
    try:
      s3cipher_obj = CortxS3Cipher(None,
                                False,
                                0,
                                self.get_confkey('CONFSTORE_OPENLDAP_CONST_KEY'))

      cipher_key = s3cipher_obj.generate_key()

      self.ldap_root_user = self.get_confvalue(self.get_confkey('CONFIG>CONFSTORE_ROOTDN_USER_KEY'))

      encrypted_rootdn_pass = self.get_confvalue(self.get_confkey('CONFIG>CONFSTORE_ROOTDN_PASSWD_KEY'))

      if encrypted_rootdn_pass != None:
          self.rootdn_passwd = s3cipher_obj.decrypt(cipher_key, encrypted_rootdn_pass)

      op_file = "/opt/seagate/cortx/s3/s3backgrounddelete/s3_cluster.yaml"

      key = 'cluster_config>rootdn_user'
      opfileconfstore = S3CortxConfStore(f'yaml://{op_file}', 'write_rootdn_idx')
      opfileconfstore.set_config(f'{key}', f'{self.ldap_root_user}', True)
      updated_rootdn_user = opfileconfstore.get_config(f'{key}')

      if updated_rootdn_user != self.ldap_root_user:
          raise S3PROVError(f'set_config failed to set {key}: {self.ldap_root_user} in {op_file} ')

      key = 'cluster_config>rootdn_pass'
      opfileconfstore.set_config(f'{key}', f'{self.rootdn_passwd}', True)
      updated_rootdn_pass = opfileconfstore.get_config(f'{key}')

      if updated_rootdn_pass != self.rootdn_passwd:
          raise S3PROVError(f'set_config failed to set {key}: {self.rootdn_passwd} in {op_file} ')

    except Exception as e:
      self.logger.error(f'write rootdn credentials failed, error: {e}')
      raise e

  def update_cluster_id(self, op_file: str = "/opt/seagate/cortx/s3/s3backgrounddelete/s3_cluster.yaml"):
    """Set 'cluster_id' to op_file."""
    try:
      if path.isfile(f'{op_file}') == False:
        raise S3PROVError(f'{op_file} must be present')
      else:
        key = 'cluster_config>cluster_id'
        opfileconfstore = S3CortxConfStore(f'yaml://{op_file}', 'write_cluster_id_idx')
        opfileconfstore.set_config(f'{key}', f'{self.cluster_id}', True)
        updated_cluster_id = opfileconfstore.get_config(f'{key}')

        if updated_cluster_id != self.cluster_id:
          raise S3PROVError(f'set_config failed to set {key}: {self.cluster_id} in {op_file} ')
    except Exception as e:
      raise S3PROVError(f'exception: {e}')

  def validate_pre_requisites(self,
                        rpms: list = None,
                        pip3s: list = None,
                        services: list = None,
                        files: list = None):
    """Validate pre requisites using cortx-py-utils validator."""
    self.logger.info(f'Validations running from {self._preqs_conf_file}')
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
      raise S3PROVError(f'ERROR: {phase_name} prereqs validations failed, exception: {e} ')

  def key_value_verify(self, key: str):
    """Verify if there exists a corresponding value for given key."""
    # Once a key from yardstick file has found a
    # matching pair in argument file, the value
    # of that key from argument file needs to be
    # verified. It should be neither none, empty
    # nor any undesirable value.
    value = self.get_confvalue(key)
    if not value:
      raise Exception(f'Empty value for key : {key}')
    else:
      address_token = ["hostname", "public_fqdn", "private_fqdn"]
      for token in address_token:
        if key.find(token) != -1:
          NetworkV().validate('connectivity',[value])
          break

  def extract_yardstick_list(self, phase_name: str):
    """Extract keylist to be used as yardstick for validating keys of each phase."""
    # The s3 prov config file has below pairs :
    # "Key Constant" : "Actual Key"
    # Example of "Key Constant" :
    #   CONFSTORE_SITE_COUNT_KEY
    #   PREPARE
    #   CONFIG>CONFSTORE_LDAPADMIN_USER_KEY
    #   INIT
    # Example of "Actual Key" :
    #   cluster>cluster-id>site>storage_set_count
    #   cortx>software>openldap>sgiam>user
    #
    # When we call get_all_keys on s3 prov config
    # file, it returns all the "Key Constant",
    # which will contain PHASE(name) as the root
    # attribute (except for unsolicited keys).
    # To get "Actual Key" from each "Key Constant",
    # we need to call get_confkey on every such key.
    #
    # Note that for each of these "Key Constant",
    # there may not exist an "Actual Key" because
    # some phases do not have any "Actual Key".
    # Example of such cases -
    #   POST_INSTALL
    #   PREPARE
    # For such examples, we skip and continue with
    # remaining keys.

    prov_keys_list = self._s3_confkeys_store.get_all_keys()
    # We have all "Key Constant" in prov_keys_list,
    # now extract "Actual Key" if it exists and
    # depending on phase and hierarchy, decide
    # whether it should be added to the yardstick
    # list for the phase passed here.
    yardstick_list = []
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
      yardstick_list.append(value)
    return yardstick_list

  def phase_keys_validate(self, arg_file: str, phase_name: str):
    """Validate keys of each phase derived from s3_prov_config and compare with argument file."""
    # Setting the desired values before we begin
    token_list = ["machine-id", "cluster-id", "storage-set-count"]
    if self.machine_id is not None:
      machine_id_val = self.machine_id
    if self.cluster_id is not None:
      cluster_id_val = self.cluster_id
    # The 'storage_set_count' is read using
    # below hard-coded key which is the max
    # array size for storage set.
    storage_set_count_key = "cluster>cluster-id>site>storage_set_count"
    if self.cluster_id is not None:
      storage_set_count_key = storage_set_count_key.replace("cluster-id", cluster_id_val)
    storage_set_count_str = self.get_confvalue(storage_set_count_key)
    if storage_set_count_str is not None:
      storage_set_val = int(storage_set_count_str)
    else:
      storage_set_val = 0
    # Set phase name to upper case required for inheritance
    phase_name = phase_name.upper()
    try:
      # Extract keys from yardstick file for current phase considering inheritance
      yardstick_list = self.extract_yardstick_list(phase_name)

      # Set argument file confstore
      argument_file_confstore = S3CortxConfStore(arg_file, 'argument_file_index')
      # Extract keys from argument file
      arg_keys_list = argument_file_confstore.get_all_keys()
      # Since get_all_keys misses out listing entries inside
      # an array, the below code is required to fetch such
      # array entries. The result will be stored in a full
      # list which will be complete and will be used to verify
      # keys required for each phase.
      full_arg_keys_list = []
      for key in arg_keys_list:
        if ((key.find('[') != -1) and (key.find(']') != -1)):
          storage_set = self.get_confvalue(key)
          base_key = key
          for set_key in storage_set:
            key = base_key + ">" + set_key
            full_arg_keys_list.append(key)
        else:
          full_arg_keys_list.append(key)

      # Below algorithm uses tokenization
      # of both yardstick and argument key
      # based on delimiter to generate
      # smaller key-tokens. Then check if
      # (A) all the key-tokens are pairs of
      #     pre-defined token. e.g.,
      #     if key_yard is machine-id, then
      #     key_arg must have corresponding
      #     value of machine_id_val.
      # OR
      # (B) both the key-tokens from key_arg
      #     and key_yard are the same.
      list_match_found = True
      key_match_found = False
      for key_yard in yardstick_list:
        key_yard_token_list = re.split('>|\[|\]',key_yard)
        key_match_found = False
        for key_arg in full_arg_keys_list:
          if key_match_found is False:
            key_arg_token_list = re.split('>|\[|\]',key_arg)
            if len(key_yard_token_list) == len(key_arg_token_list):
              for key_x,key_y in zip(key_yard_token_list, key_arg_token_list):
                key_match_found = False
                if key_x == "machine-id":
                  if key_y != machine_id_val:
                    break
                elif key_x == "cluster-id":
                  if key_y != cluster_id_val:
                    break
                elif key_x == "storage-set-count":
                  if int(key_y) >= storage_set_val:
                    break
                elif key_x != key_y:
                  break
                key_match_found = True
              if key_match_found:
                self.key_value_verify(key_arg)
        if key_match_found is False:
          list_match_found = False
          break
      if list_match_found is False:
        raise Exception(f'No match found for {key_yard}')
      self.logger.info("Validation complete")

    except Exception as e:
      raise Exception(f'ERROR : Validating keys failed, exception {e}')

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
      self.logger.info(f"shutting down {service_name}")
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
      self.logger.info(f"starting {service_name}")
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
      self.logger.info(f"restarting {service_name}")
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
      self.logger.info(f"reloading {service_name}")
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

  def validate_config_files(self, phase_name: str):
    """Validate the sample file and config file keys.
    Both files should have same keys.
    if keys mismatch then there is some issue in the config file."""

    self.logger.info(f'validating S3 config files for {phase_name}.')
    upgrade_items = {
    's3' : {
          'configFile' : "/opt/seagate/cortx/s3/conf/s3config.yaml",
          'SampleFile' : "/opt/seagate/cortx/s3/conf/s3config.yaml.sample",
          'fileType' : 'yaml://'
      },
      'auth' : {
          'configFile' : "/opt/seagate/cortx/auth/resources/authserver.properties",
          'SampleFile' : "/opt/seagate/cortx/auth/resources/authserver.properties.sample",
          'fileType' : 'properties://'
      },
      'keystore' : {
          'configFile' : "/opt/seagate/cortx/auth/resources/keystore.properties",
          'SampleFile' : "/opt/seagate/cortx/auth/resources/keystore.properties.sample",
          'fileType' : 'properties://'
      },
      'bgdelete' : {
          'configFile' : "/opt/seagate/cortx/s3/s3backgrounddelete/config.yaml",
          'SampleFile' : "/opt/seagate/cortx/s3/s3backgrounddelete/config.yaml.sample",
          'fileType' : 'yaml://'
      },
      'cluster' : {
          'configFile' : "/opt/seagate/cortx/s3/s3backgrounddelete/s3_cluster.yaml",
          'SampleFile' : "/opt/seagate/cortx/s3/s3backgrounddelete/s3_cluster.yaml.sample",
          'fileType' : 'yaml://'
      }
    }

    for upgrade_item in upgrade_items:
      configFile = upgrade_items[upgrade_item]['configFile']
      SampleFile = upgrade_items[upgrade_item]['SampleFile']
      filetype = upgrade_items[upgrade_item]['fileType']
      self.logger.info(f'validating config file {str(configFile)}.')

      # new sample file
      conf_sample = filetype + SampleFile
      cs_conf_sample = S3CortxConfStore(config=conf_sample, index=conf_sample + "validator")
      conf_sample_keys = cs_conf_sample.get_all_keys()

      # active config file
      conf_file =  filetype + configFile
      cs_conf_file = S3CortxConfStore(config=conf_file, index=conf_file + "validator")
      conf_file_keys = cs_conf_file.get_all_keys()

      # compare the keys of sample file and config file
      if conf_sample_keys.sort() == conf_file_keys.sort():
          self.logger.info(f'config file {str(configFile)} validated successfully.')
      else:
          self.logger.error(f'config file {str(conf_file)} and sample file {str(conf_sample)} keys does not matched.')
          self.logger.error(f'sample file keys: {str(conf_sample_keys)}')
          self.logger.error(f'config file keys: {str(conf_file_keys)}')
          raise Exception(f'ERROR: Failed to validate config file {str(configFile)}.')

  def DeleteDirContents(self, dirname: str,  skipdirs: list = []):
    """Delete files and directories inside given directory.
    It will skips the directories which are part of skipdirs list.
    """
    if os.path.exists(dirname):
      for filename in os.listdir(dirname):
        filepath = os.path.join(dirname, filename)
        try:
          if os.path.isfile(filepath):
            os.remove(filepath)
          elif os.path.isdir(filepath):
            if filepath in skipdirs:
              self.logger.info(f'Skipping the dir {filepath}')
            else:
              shutil.rmtree(filepath)
        except Exception as e:
          self.logger.error(f'ERROR: DeleteDirContents(): Failed to delete: {filepath}, error: {str(e)}')
          raise e

  def DeleteFile(self, filepath: str):
    """Delete file."""
    if os.path.exists(filepath):
      try:
        os.remove(filepath)
      except Exception as e:
        self.logger.error(f'ERROR: DeleteFile(): Failed to delete file: {filepath}, error: {str(e)}')
        raise e

  def DeleteFileOrDirWithRegex(self, path: str, regex: str):
    """Delete files and directories inside given directory for which regex matches."""
    if os.path.exists(path):
      filepath = os.path.join(path, regex)
      files = glob.glob(filepath)
      for file in files:
        try:
          if os.path.isfile(file):
            os.remove(file)
          elif os.path.isdir(file):
            shutil.rmtree(file)
        except Exception as e:
          self.logger.error(f'ERROR: DeleteFileOrDirWithRegex(): Failed to delete: {file}, error: {str(e)}')
          raise e