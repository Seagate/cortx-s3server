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
import glob
import socket
import ldap
import uuid
import urllib
from os import path
from ast import literal_eval
from s3confstore.cortx_s3_confstore import S3CortxConfStore
from s3cipher.cortx_s3_cipher import CortxS3Cipher
from cortx.utils.validator.v_pkg import PkgV
from cortx.utils.validator.v_service import ServiceV
from cortx.utils.validator.v_path import PathV
from cortx.utils.validator.v_network import NetworkV
from cortx.utils.process import SimpleProcess
from cortx.utils.cortx.const import Const
from cortx.utils.log import Log

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
  s3_prov_config = "/opt/seagate/cortx/s3/mini-prov/s3_prov_config.yaml"

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

  def __init__(self,config: str, services: str):
    """Constructor."""
    self.services = services
    self.endpoint = None
    self._url = None
    self._provisioner_confstore = None
    self._s3_confkeys_store = None
    self.machine_id = None
    self.cluster_id = None
    self.base_config_file_path = "/etc/cortx"
    self.base_log_file_path = "/var/log/cortx"
    self.consul_confstore = None

    # Constant service names from utils.
    self.service_haproxy = Const.SERVICE_S3_HAPROXY.value
    self.service_s3server = Const.SERVICE_S3_SERVER.value
    self.service_authserver = Const.SERVICE_S3_AUTHSERVER.value
    self.service_bgscheduler = Const.SERVICE_S3_BGSCHEDULER.value
    self.service_bgworker = Const.SERVICE_S3_BGWORKER.value

    # validate supported services
    lookup_service = [self.service_haproxy, self.service_s3server, self.service_authserver, self.service_bgscheduler, self.service_bgworker]
    if self.services is None:
      self.services = str(f"{self.service_haproxy},{self.service_s3server},{self.service_authserver},{self.service_bgscheduler},{self.service_bgworker}")

    self.bg_delete_service = self.service_bgworker
    if -1 != self.services.find("bg_"):
      self.bg_delete_service = "bg_consumer"
    # follwing mapping needs to be removed once the services names are changed in provisioner and solution framework
    ######### start
    services_map = {
                "io": str(f"{self.service_s3server},{self.service_haproxy}"),
                "auth": self.service_authserver,
                "bg_producer": self.service_bgscheduler,
                "bg_consumer": self.service_bgworker}
    for service in services_map:
        if -1 != self.services.find(service):
          if (service == "auth") and (-1 != services.find(self.service_authserver)):
            continue
          self.services = self.services.replace(service, services_map[service])
    ######### End

    self.services = self.services.split(",")

    # follwing mapping needs to be removed once the services names are changed in provisioner and solution framework
    ######### start
    if "openldap" in  self.services:
         self.services.remove("openldap")
    ######### End

    for service in self.services:
      if service not in lookup_service:
        raise Exception(f'ERROR: {service} service is not supported.')

    self._s3_confkeys_store = S3CortxConfStore(f'yaml://{self.s3_prov_config}', 'setup_s3keys_index')

    # get all the param from the s3_prov_config file
    self._preqs_conf_file = self.get_confkey('VALIDATION_PREREQ_FILE')
    self.ldap_mdb_folder = self.get_confkey('LDAP_MDB_LOCATION')

    # Get machine-id of current node from constore
    self.machine_id = self._s3_confkeys_store.get_machine_id()
    Log.info(f'Machine id : {self.machine_id}')

    if config is None:
      Log.warn(f'Empty Config url')
      return

    if not config.strip():
      Log.error(f'Config url:[{config}] must be a valid url path')
      raise Exception('Empty config URL path')

    self._url = config
    self._provisioner_confstore = S3CortxConfStore(self._url, 'setup_prov_index')
    self.base_config_file_path = self.get_confvalue_with_defaults('CONFIG>CONFSTORE_BASE_CONFIG_PATH')
    Log.info(f'config file path : {self.base_config_file_path}')
    self.s3_tmp_dir = os.path.join(self.base_config_file_path, "s3/tmp")
    Log.info(f'tmp dir : {self.s3_tmp_dir}')

    # Consul URL 
    try:
        consul_endpoint_url=self.get_endpoint("CONFIG>CONFSTORE_CONSUL_ENDPOINTS", "fqdn", "http")
        consul_endpoint_port=self.get_endpoint("CONFIG>CONFSTORE_CONSUL_ENDPOINTS", "port", "http")
        consul_protocol='consul://'
        # consul url will be : consul://consul-server.default.svc.cluster.local:8500
        consul_url= f'{consul_protocol}'+ f'{consul_endpoint_url}' + ':' + f'{consul_endpoint_port}'
    except S3PROVError:
        # endpoint entry is not found in confstore hence fetch endpoint url from default value.
        consul_url=self.get_confvalue_with_defaults("DEFAULT_CONFIG>CONFSTORE_CONSUL_ENDPOINTS")
        Log.info(f'consul endpoint url entry (http://<consul-fqdn>:<port>) is missing for protocol type: http from confstore, hence using default value as {consul_url}')

    Log.info(f'loading consul service with consul endpoint URL as:{consul_url}')
    self.consul_confstore = S3CortxConfStore(config=f'{consul_url}', index=str(uuid.uuid1()))

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

  def get_confvalue_with_defaults(self, key: str):
    assert self.provisioner_confstore != None
    if key.startswith("DEFAULT_"):
      return self.s3_confkeys_store.get_config(key)
    conf_key = self.get_confkey(key)
    if conf_key is not None:
      if "machine-id" in conf_key:
        conf_key = conf_key.replace("machine-id", self.machine_id)
      if "cluster-id" in conf_key:
        if self.cluster_id is not None:
          conf_key = conf_key.replace("cluster-id", self.cluster_id)
      value = self.provisioner_confstore.get_config(conf_key)
      if value is None:
        value = self.s3_confkeys_store.get_config("DEFAULT_"+ key)
      return value
    return None

  def search_confvalue(self, parent_key:str, key_to_search:str, value_to_search: str):
    assert self.provisioner_confstore != None
    return self.provisioner_confstore.search(parent_key, key_to_search, value_to_search)

  def read_endpoint_value(self):
    if self.endpoint is None:
      self.endpoint = self.get_confvalue_with_defaults('TEST>TEST_CONFSTORE_ENDPOINT_KEY')

  def read_ldap_credentials_for_test_phase(self):
    """Get 'ldapadmin' user name and plaintext password from confstore for test phase."""
    try:
      self.ldap_user = self.get_confvalue_with_defaults('TEST>TEST_CONFSTORE_LDAPADMIN_USER_KEY')
      self.ldap_passwd = self.get_confvalue_with_defaults('TEST>TEST_CONFSTORE_LDAPADMIN_PASSWD_KEY')
    except Exception as e:
      sys.stderr.write(f'read ldap credentials failed, error: {e}\n')
      raise e

  def read_ldap_credentials(self):
    """Get 'ldapadmin' user name and password from confstore."""
    try:
      s3cipher_obj = CortxS3Cipher(None,
                                False,
                                0,
                                self.get_confkey('CONFSTORE_OPENLDAP_CONST_KEY'))

      cipher_key = s3cipher_obj.generate_key()

      self.ldap_user = self.get_confvalue_with_defaults('CONFIG>CONFSTORE_LDAPADMIN_USER_KEY')

      encrypted_ldapadmin_pass = self.get_confvalue_with_defaults('CONFIG>CONFSTORE_LDAPADMIN_PASSWD_KEY')

      if encrypted_ldapadmin_pass != None:
        self.ldap_passwd = s3cipher_obj.decrypt(cipher_key, encrypted_ldapadmin_pass)

    except Exception as e:
      Log.error(f'read ldap credentials failed, error: {e}')
      raise e

  def update_rootdn_credentials(self, s3_cluster_file : str):
    """Set rootdn username and password to opfile."""
    try:
      s3cipher_obj = CortxS3Cipher(None,
                                False,
                                0,
                                self.get_confkey('CONFSTORE_OPENLDAP_CONST_KEY'))

      cipher_key = s3cipher_obj.generate_key()

      self.ldap_root_user = self.get_confvalue_with_defaults('CONFIG>CONFSTORE_ROOTDN_USER_KEY')

      encrypted_rootdn_pass = self.get_confvalue_with_defaults('CONFIG>CONFSTORE_ROOTDN_PASSWD_KEY')

      if encrypted_rootdn_pass is not None:
        self.rootdn_passwd = s3cipher_obj.decrypt(cipher_key, encrypted_rootdn_pass)

      if encrypted_rootdn_pass is None:
        raise S3PROVError('password cannot be None.')

      key = 'cluster_config>rootdn_user'
      opfileconfstore = S3CortxConfStore(f'yaml://{s3_cluster_file}', str(uuid.uuid1()))
      opfileconfstore.set_config(f'{key}', f'{self.ldap_root_user}', True)

      key = 'cluster_config>rootdn_pass'
      opfileconfstore.set_config(f'{key}', f'{encrypted_rootdn_pass}', True)

    except Exception as e:
      Log.error(f'update rootdn credentials failed, error: {e}')
      raise e

  def update_cluster_id(self, op_file: str):
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
    Log.info(f'Validations running from {self._preqs_conf_file}')
    if pip3s:
      PkgV().validate('pip3s', pip3s)
    if services:
      for service in services:
        pid = os.popen('pidof '+service).read()
        if pid is None:
          raise Exception('Validation failed for service %s' % (service))
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

  def extract_yardstick_list(self, phase_name: str):
    """Extract keylist to be used as yardstick for validating keys of each phase."""
    # The s3 prov config file has below pairs :
    # "Key Constant" : "Actual Key"
    # Example of "Key Constant" :
    #   PREPARE
    #   CONFIG>CONFSTORE_LDAPADMIN_USER_KEY
    #   INIT
    # Example of "Actual Key" :
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
    
    # map to identify which all keys to validate from phases
    phase_list = {
      "POST_INSTALL": ["POST_INSTALL"],
      "PREPARE" : ["POST_INSTALL", "PREPARE"],
      "CONFIG" : ["POST_INSTALL", "PREPARE", "CONFIG"],
      "INIT" : ["POST_INSTALL", "PREPARE", "CONFIG", "INIT"],
      "RESET": ["RESET"],
      "CLEANUP": ["CLEANUP"],
      "TEST" : ["TEST"]}

    # get all phases required to validate on current phase name
    phases_to_validate = phase_list[phase_name]

    # Get all keys from the s3_prov_config.yaml
    prov_keys_list = self._s3_confkeys_store.get_all_keys()

    # We have all "Key Constant" in prov_keys_list,
    # now extract "Actual Key" if it exists and
    # depending on phase and hierarchy, decide
    # whether it should be added to the yardstick
    # list for the phase passed here.
    yardstick_list = []
    for phase in phases_to_validate:
      for key in prov_keys_list:
        # comparing phase name with '>' to match exact key
        if key.find(phase + ">") == 0:
          # Append default to key & check if it has a value
          # If value does not exist which can be the
          # case for certain phases as mentioned above,
          # skip the value.
          default_key = "DEFAULT_" + key
          value = self.get_confvalue_with_defaults(default_key)
          if value is not None:
            continue
          yardstick_list.append(self.get_confkey(key))
    return yardstick_list

  def phase_keys_validate(self, arg_file: str, phase_name: str):
    """Validate keys of each phase derived from s3_prov_config and compare with argument file."""
    # Set phase name to upper case required for inheritance
    phase_name = phase_name.upper()
    # Extract keys from yardstick file for current phase considering inheritance
    yardstick_list = self.extract_yardstick_list(phase_name)
    Log.info(f"yardstick_list -> {yardstick_list}")
    # Set argument file confstore
    argument_file_confstore = S3CortxConfStore(arg_file, 'argument_file_index')
    # Extract keys from argument file
    arg_keys_list = argument_file_confstore.get_all_keys(key_index = False)
    Log.info(f"template_list -> {arg_keys_list}")
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
    for key_yard in yardstick_list:
      if "machine-id" in key_yard:
        key_yard = key_yard.replace("machine-id", self.machine_id)
      if "cluster-id" in key_yard:
        key_yard = key_yard.replace("cluster-id", self.cluster_id)
      if "node-id" in key_yard:
        key_yard = key_yard.replace("node-id", self.machine_id)
      if "nodes" in key_yard:
        storage_set_count = self.get_confvalue_with_defaults('CONFIG>CONFSTORE_STORAGE_SET_COUNT')
        Log.info(f"storage_set_count : {storage_set_count}")
        index = 0
        while index < int(storage_set_count):
          key_yard_server_nodes_key = key_yard.replace("storage_set_count", str(index))
          Log.info(f"key_yard_server_nodes_key : {key_yard_server_nodes_key}")
          key_yard_server_nodes = self.get_confvalue(key_yard_server_nodes_key)
          if key_yard_server_nodes is None:
            raise Exception("Validation for server_nodes failed")
          index += 1
      else:
        if key_yard in arg_keys_list:
          self.key_value_verify(key_yard)
        else:
          raise Exception(f'No match found for {key_yard}')
    Log.info("Validation complete")

  def shutdown_services(self, s3services_list):
    """Stop services."""
    for service_name in s3services_list:
      cmd = ['/bin/systemctl', 'is-active',  f'{service_name}']
      handler = SimpleProcess(cmd)
      Log.info(f"Check {service_name} service is active or not ")
      res_op, res_err, res_rc = handler.run()
      if res_rc == 0:
        Log.info(f"Service {service_name} is active")
        try:
          # if service name not found in the ha_service_map then use systemctl
          service_name = self.ha_service_map[service_name]
          cmd = ['cortx', 'stop',  f'{service_name}']
        except KeyError:
          cmd = ['/bin/systemctl', 'stop',  f'{service_name}']
        Log.info(f"Command: {cmd}")
        handler = SimpleProcess(cmd)
        res_op, res_err, res_rc = handler.run()
        if res_rc != 0:
          raise Exception(f"{cmd} failed with err: {res_err}, out: {res_op}, ret: {res_rc}")
      else:
        Log.info(f"Service {service_name} is not active")

  def start_services(self, s3services_list):
    """Start services specified as parameter."""
    for service_name in s3services_list:
      cmd = ['/bin/systemctl', 'is-active',  f'{service_name}']
      handler = SimpleProcess(cmd)
      Log.info(f"Check {service_name} service is active or not ")
      res_op, res_err, res_rc = handler.run()
      if res_rc != 0:
        Log.info(f"Service {service_name} is not active")
        try:
          # if service name not found in the ha_service_map then use systemctl
          service_name = self.ha_service_map[service_name]
          cmd = ['cortx', 'start',  f'{service_name}']
        except KeyError:
          cmd = ['/bin/systemctl', 'start',  f'{service_name}']
        Log.info(f"Command: {cmd}")
        handler = SimpleProcess(cmd)
        res_op, res_err, res_rc = handler.run()
        if res_rc != 0:
          raise Exception(f"{cmd} failed with err: {res_err}, out: {res_op}, ret: {res_rc}")
      else:
        Log.info(f"Service {service_name} is already active")

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
      Log.info(f"restarting {service_name}")
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
      Log.info(f"reloading {service_name}")
      res_op, res_err, res_rc = handler.run()
      if res_rc != 0:
        raise Exception(f"{cmd} failed with err: {res_err}, out: {res_op}, ret: {res_rc}")

  def disable_services(self, s3services_list):
    """Disable services specified as parameter."""
    for service_name in s3services_list:
      cmd = ['/bin/systemctl', 'disable', f'{service_name}']
      handler = SimpleProcess(cmd)
      Log.info(f"Disabling {service_name}")
      res_op, res_err, res_rc = handler.run()
      if res_rc != 0:
        raise Exception(f"{cmd} failed with err: {res_err}, out: {res_op}, ret: {res_rc}")

  def validate_config_files(self, phase_name: str):
    """Validate the sample file and config file keys.
    Both files should have same keys.
    if keys mismatch then there is some issue in the config file."""

    Log.info(f'validating S3 config files for {phase_name}.')
    upgrade_items = {
    's3' : {
          'configFile' : self.get_confkey('S3_CONFIG_FILE').replace("/opt/seagate/cortx", self.base_config_file_path),
          'SampleFile' : self.get_confkey('S3_CONFIG_SAMPLE_FILE').replace("/opt/seagate/cortx", self.base_config_file_path),
          'fileType' : 'yaml://'
      },
      'auth' : {
          'configFile' : self.get_confkey('S3_AUTHSERVER_CONFIG_FILE').replace("/opt/seagate/cortx", self.base_config_file_path),
          'SampleFile' : self.get_confkey('S3_AUTHSERVER_CONFIG_SAMPLE_FILE').replace("/opt/seagate/cortx", self.base_config_file_path),
          'fileType' : 'properties://'
      },
      'keystore' : {
          'configFile' : self.get_confkey('S3_KEYSTORE_CONFIG_FILE').replace("/opt/seagate/cortx", self.base_config_file_path),
          'SampleFile' : self.get_confkey('S3_KEYSTORE_CONFIG_SAMPLE_FILE').replace("/opt/seagate/cortx", self.base_config_file_path),
          'fileType' : 'properties://'
      },
      'bgdelete' : {
          'configFile' : self.get_confkey('S3_BGDELETE_CONFIG_FILE').replace("/opt/seagate/cortx", self.base_config_file_path),
          'SampleFile' : self.get_confkey('S3_BGDELETE_CONFIG_SAMPLE_FILE').replace("/opt/seagate/cortx", self.base_config_file_path),
          'fileType' : 'yaml://'
      },
      'cluster' : {
          'configFile' : self.get_confkey('S3_CLUSTER_CONFIG_FILE').replace("/opt/seagate/cortx", self.base_config_file_path),
          'SampleFile' : self.get_confkey('S3_CLUSTER_CONFIG_SAMPLE_FILE').replace("/opt/seagate/cortx", self.base_config_file_path),
          'fileType' : 'yaml://'
      }
    }

    for upgrade_item in upgrade_items:
      self.validate_config_file(upgrade_items[upgrade_item]['configFile'],
                              upgrade_items[upgrade_item]['SampleFile'],
                              upgrade_items[upgrade_item]['fileType'])

  def validate_config_file(self, configFile: str, SampleFile: str, filetype: str):
    """Validate the sample file and config file keys.
    Both files should have same keys.
    if keys mismatch then there is some issue in the config file."""

    Log.info(f'validating config file {str(configFile)}.')

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
        Log.info(f'config file {str(configFile)} validated successfully.')
    else:
        Log.error(f'config file {str(conf_file)} and sample file {str(conf_sample)} keys does not matched.')
        Log.error(f'sample file keys: {str(conf_sample_keys)}')
        Log.error(f'config file keys: {str(conf_file_keys)}')
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
              Log.info(f'Skipping the dir {filepath}')
            else:
              shutil.rmtree(filepath)
        except Exception as e:
          Log.error(f'ERROR: DeleteDirContents(): Failed to delete: {filepath}, error: {str(e)}')
          raise e

  def DeleteFile(self, filepath: str):
    """Delete file."""
    if os.path.exists(filepath):
      try:
        os.remove(filepath)
      except Exception as e:
        Log.error(f'ERROR: DeleteFile(): Failed to delete file: {filepath}, error: {str(e)}')
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
          Log.error(f'ERROR: DeleteFileOrDirWithRegex(): Failed to delete: {file}, error: {str(e)}')
          raise e

  def get_iam_admin_credentials(self):
    """Used for reset and cleanup phase to get the iam-admin user and decrypted passwd."""
    auth_conf_file = self.get_confkey('S3_AUTHSERVER_CONFIG_FILE').replace("/opt/seagate/cortx", self.base_config_file_path)
    opfileconfstore = S3CortxConfStore(f'properties://{auth_conf_file}', 'read_ldap_idx')
    s3cipher_obj = CortxS3Cipher(None,
                              False,
                              0,
                              self.get_confkey('CONFSTORE_OPENLDAP_CONST_KEY'))

    enc_ldap_passwd = opfileconfstore.get_config('ldapLoginPW')
    cipher_key = s3cipher_obj.generate_key()

    if enc_ldap_passwd != None:
      self.ldap_passwd = s3cipher_obj.decrypt(cipher_key, enc_ldap_passwd)

  def read_ldap_root_credentials(self):
    """Used for reset and cleanup phase to get the ldap root user and decrypted passwd."""
    key = 'cluster_config>rootdn_user'

    s3_cluster_file = self.get_confkey('S3_CLUSTER_CONFIG_FILE').replace("/opt/seagate/cortx", self.base_config_file_path)
    opfileconfstore = S3CortxConfStore(f'yaml://{s3_cluster_file}', str(uuid.uuid1()))
    self.ldap_root_user = opfileconfstore.get_config(f'{key}')

    key = 'cluster_config>rootdn_pass'
    enc_rootdn_passwd = opfileconfstore.get_config(f'{key}')

    s3cipher_obj = CortxS3Cipher(None,
                            False,
                            0,
                            self.get_confkey('CONFSTORE_OPENLDAP_CONST_KEY'))
    
    cipher_key = s3cipher_obj.generate_key()

    if enc_rootdn_passwd != None:
      self.rootdn_passwd = s3cipher_obj.decrypt(cipher_key, enc_rootdn_passwd)

  def get_config_param_for_BG_delete_account(self):
    """To get the config parameters required in init and reset phase."""
    BG_delete_config_file = self.get_confkey('S3_BGDELETE_CONFIG_FILE').replace("/opt/seagate/cortx", self.base_config_file_path)
    opfileconfstore = S3CortxConfStore(f'yaml://{BG_delete_config_file}', 'read_bg_delete_config_idx')

    param_list = ['account_name', 'account_id', 'canonical_id', 'mail', 's3_user_id', 'const_cipher_secret_str', 'const_cipher_access_str']
    bgdelete_acc_input_params_dict = {}
    for param in param_list:
        key = 'background_delete_account' + '>' + param
        value_for_key = opfileconfstore.get_config(f'{key}')
        bgdelete_acc_input_params_dict[param] = value_for_key

    return bgdelete_acc_input_params_dict

  def copy_config_files(self, config_files: list):
    """ Copy config files from /opt/seagate/cortx to /etc/cortx."""
    # copy all the config files from the /opt/seagate/cortx to /etc/cortx
    for config_file in config_files:
      Log.info(f"Source config file: {config_file}")
      dest_config_file = config_file.replace("/opt/seagate/cortx", self.base_config_file_path)
      Log.info(f"Dest config file: {dest_config_file}")
      os.makedirs(os.path.dirname(dest_config_file), exist_ok=True)
      shutil.copy(config_file, dest_config_file)
      Log.info("Config file copied successfully to /etc/cortx")

  def make_sample_old_files(self, config_files: list):
    """ Copy from /opt/seagate/cortx to make '.old' files in /etc/cortx/s3/tmp."""
    # for given config files at /opt/seagate/cortx, make '.old' in /etc/cortx/s3/tmp
    for config_file in config_files:
      Log.info(f"Source config file: {config_file}")
      old_file_name = os.path.basename(config_file) + '.old'
      old_config_file = os.path.join(self.s3_tmp_dir, old_file_name)
      Log.info(f"Dest config file: {old_config_file}")
      os.makedirs(os.path.dirname(old_config_file), exist_ok=True)
      shutil.copy(config_file, old_config_file)
      Log.info("Config file copied successfully to /etc/cortx/s3/tmp")

  def modify_attribute(self, dn, attribute, value):
        # Open a connection
        ldap_conn = ldap.initialize("ldapi:///")
        # Bind/authenticate with a user with apropriate rights to add objects
        ldap_conn.sasl_non_interactive_bind_s('EXTERNAL')
        mod_attrs = [(ldap.MOD_REPLACE, attribute, bytes(str(value), 'utf-8'))]
        try:
            ldap_conn.modify_s(dn, mod_attrs)
        except:
            Log.error('Error while modifying attribute- '+ attribute )
            raise Exception('Error while modifying attribute' + attribute)
        ldap_conn.unbind_s()

  def search_and_delete_attribute(self, dn, attr_to_delete):
        conn = ldap.initialize("ldapi://")
        conn.sasl_non_interactive_bind_s('EXTERNAL')
        ldap_result_id = conn.search_s(dn, ldap.SCOPE_BASE, None, [attr_to_delete])
        total = self.get_record_count(dn, attr_to_delete)
        count = 0
        # Below will perform delete operation
        while (count < total):
            ldap_result_id = conn.search_s(dn, ldap.SCOPE_BASE, None, [attr_to_delete])
            for result1,result2 in ldap_result_id:
                if(result2):
                    for value in result2[attr_to_delete]:
                        if(value and (('dc=s3,dc=seagate,dc=com' in value.decode('UTF-8')) or ('cn=sgiamadmin,dc=seagate,dc=com' in value.decode('UTF-8')))):
                            mod_attrs = [( ldap.MOD_DELETE, attr_to_delete, value )]
                            try:
                                conn.modify_s(dn, mod_attrs)
                                break
                            except Exception as e:
                                print(e)
            count = count + 1
        conn.unbind_s()   

  def get_record_count(self, dn, attr_to_delete):
        conn = ldap.initialize("ldapi://")
        conn.sasl_non_interactive_bind_s('EXTERNAL')
        ldap_result_id = conn.search_s(dn, ldap.SCOPE_BASE, None, [attr_to_delete])
        total = 0
        # Below will count the entries
        for result1,result2 in ldap_result_id:
            if(result2):
                for value in result2[attr_to_delete]:
                    if(value and (('dc=s3,dc=seagate,dc=com' in value.decode('UTF-8')) or ('cn=sgiamadmin,dc=seagate,dc=com' in value.decode('UTF-8')))):
                        total = total + 1
        conn.unbind_s()
        return total

  def get_endpoint(self, confstore_key, expected_token,  endpoint_type):
    """1.Fetch confstore value from given key i.e. confstore_key
    2.Parse endpoint string based on expected endpoint type i.e. endpoint_type
    3.Return specific value as mentioned as per parameter i.e. expected_token.
      this expected_token must has value from ['scheme', 'fqdn', 'port']
    Examples:
    fetch_ldap_host = self.get_endpoint("CONFIG>CONFSTORE_S3_OPENLDAP_ENDPOINTS", "fqdn", "ldap")
    fetch_ldap_port = self.get_endpoint("CONFIG>CONFSTORE_S3_OPENLDAP_ENDPOINTS", "port", "ldap")
    """
    confstore_key_value = self.get_confvalue_with_defaults(confstore_key)
    # Checking if the value is a string or not.
    if isinstance(confstore_key_value, str):
      confstore_key_value = literal_eval(confstore_key_value)

    # Checking if valid token name is expected or not.
    allowed_token_list = ['scheme', 'fqdn', 'port']
    if not expected_token in allowed_token_list:
      raise S3PROVError(f"Incorrect token string {expected_token} received for {confstore_key} for specified endpoint type : {endpoint_type}")

    endpoint = self.get_endpoint_for_scheme(confstore_key_value, endpoint_type)
    if endpoint is None:
      raise S3PROVError(f"{confstore_key} does not have any specified endpoint type : {endpoint_type}")
    if expected_token not in endpoint:
      raise S3PROVError(f"{confstore_key} does not specify endpoint fqdn {endpoint} for endpoint type {endpoint_type}")
    return endpoint[expected_token]

  def get_endpoint_for_scheme(self, value_to_update, scheme):
    """Scan list of endpoints, and return parsed endpoint for a given scheme."""
    if not isinstance(value_to_update, str):
      lst=value_to_update
    else:
      lst=[value_to_update]
    for endpoint_str in lst:
      endpoint = self.parse_endpoint(endpoint_str)
      if endpoint['scheme'] == scheme:
        return endpoint
    return None

  def parse_endpoint(self, endpoint_str):
    """Parse endpoint string and return dictionary with components:
      * scheme,
      * fqdn,
      * and optionally port, if present in the string.

    Examples:

    'https://s3.seagate.com:443' -> {'scheme': 'https', 'fqdn': 's3.seagate.com', 'port': '443'}
    'https://s3.seagate.com'     -> {'scheme': 'https', 'fqdn': 's3.seagate.com'}
    'http://s3.seagate.com:80'   -> {'scheme': 'http', 'fqdn': 's3.seagate.com', 'port': '80'}
    'http://127.0.0.1:80'        -> {'scheme': 'http', 'fqdn': '127.0.0.1', 'port': '80'}
    """
    try:
      result1 = urllib.parse.urlparse(endpoint_str)
      result2 = result1.netloc.split(':')
      result = { 'scheme': result1.scheme, 'fqdn': result2[0] }
      if len(result2) > 1:
        result['port'] = result2[1]
    except Exception as e:
      raise S3PROVError(f'Failed to parse endpoing {endpoint_str}.  Exception: {e}')
    return result
