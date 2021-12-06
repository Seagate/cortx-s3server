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
import time
import errno
import shutil
import math
import urllib
import fcntl
import glob
import uuid
from pathlib import Path
from  ast import literal_eval
from os import path
from s3confstore.cortx_s3_confstore import S3CortxConfStore
from setupcmd import SetupCmd, S3PROVError
from cortx.utils.process import SimpleProcess
from s3msgbus.cortx_s3_msgbus import S3CortxMsgBus
from s3backgrounddelete.cortx_s3_config import CORTXS3Config
from s3backgrounddelete.cortx_s3_constants import MESSAGE_BUS
from s3_haproxy_config import S3HaproxyConfig
from ldapaccountaction import LdapAccountAction
from s3cipher.cortx_s3_cipher import CortxS3Cipher
import xml.etree.ElementTree as ET

class ConfigCmd(SetupCmd):
  """Config Setup Cmd."""
  name = "config"

  def __init__(self, config: str, services: str = None):
    """Constructor."""
    try:
      super(ConfigCmd, self).__init__(config, services)
      self.setup_type = self.get_confvalue_with_defaults('CONFIG>CONFSTORE_SETUP_TYPE')
      self.logger.info(f'Setup type : {self.setup_type}')
      self.cluster_id = self.get_confvalue_with_defaults('CONFIG>CONFSTORE_CLUSTER_ID_KEY')
      self.logger.info(f'Cluster  id : {self.cluster_id}')
      self.base_config_file_path = self.get_confvalue_with_defaults('CONFIG>CONFSTORE_BASE_CONFIG_PATH')
      self.logger.info(f'config file path : {self.base_config_file_path}')
      self.base_log_file_path = self.get_confvalue_with_defaults('CONFIG>CONFSTORE_BASE_LOG_PATH')
      self.logger.info(f'log file path : {self.base_log_file_path}')

    except Exception as e:
      raise S3PROVError(f'exception: {e}')

  def process(self, *args, **kwargs):
    lock_directory = os.path.join(self.base_config_file_path,"s3")
    if not os.path.isdir(lock_directory):
      try:
         os.mkdir(lock_directory)
      except BaseException:
         self.logger.error("Unable to create lock_directory directory ")
    lockfile = path.join(lock_directory, 's3_setup.lock')
    self.logger.info(f'Acquiring the lock at {lockfile}...')
    with open(lockfile, 'w') as lock:
      fcntl.flock(lock, fcntl.LOCK_EX)
      self.logger.info(f'acquired the lock at {lockfile}.')
      self.process_under_flock(*args, **kwargs)
    # lock and file descriptor released automatically here.
    self.logger.info(f'released the lock at {lockfile}.')

  def process_under_flock(self, skip_haproxy = False):
    """Main processing function."""
    self.logger.info(f"Processing phase = {self.name}, config = {self.url}, service = {self.services}")
    self.logger.info("validations started")
    self.phase_prereqs_validate(self.name)
    self.phase_keys_validate(self.url, self.name)
    self.logger.info("validations completed")

    try:

      # copy config files from /opt/seagate to base dir of config files (/etc/cortx)
      self.logger.info("copy config files started")
      self.copy_config_files([self.get_confkey('S3_CONFIG_FILE'),
                    self.get_confkey('S3_CONFIG_SAMPLE_FILE'),
                    self.get_confkey('S3_CONFIG_UNSAFE_ATTR_FILE'),
                    self.get_confkey('S3_AUTHSERVER_CONFIG_FILE'),
                    self.get_confkey('S3_AUTHSERVER_CONFIG_SAMPLE_FILE'),
                    self.get_confkey('S3_AUTHSERVER_CONFIG_UNSAFE_ATTR_FILE'),
                    self.get_confkey('S3_KEYSTORE_CONFIG_FILE'),
                    self.get_confkey('S3_KEYSTORE_CONFIG_SAMPLE_FILE'),
                    self.get_confkey('S3_KEYSTORE_CONFIG_UNSAFE_ATTR_FILE'),
                    self.get_confkey('S3_BGDELETE_CONFIG_FILE'),
                    self.get_confkey('S3_BGDELETE_CONFIG_SAMPLE_FILE'),
                    self.get_confkey('S3_BGDELETE_CONFIG_UNSAFE_ATTR_FILE'),
                    self.get_confkey('S3_CLUSTER_CONFIG_FILE'),
                    self.get_confkey('S3_CLUSTER_CONFIG_SAMPLE_FILE'),
                    self.get_confkey('S3_CLUSTER_CONFIG_UNSAFE_ATTR_FILE')])
      self.logger.info("copy config files completed")

      # copy s3 authserver resources to base dir of config files (/etc/cortx)
      self.logger.info("copy s3 authserver resources started")
      self.copy_s3authserver_resources()
      self.logger.info("copy s3 authserver resources completed")

      if "K8" != str(self.get_confvalue_with_defaults('CONFIG>CONFSTORE_SETUP_TYPE')):
        # Copy log rotation config files from install directory to cron directory.
        self.logger.info("copy log rotate config started")
        self.copy_logrotate_files()
        self.logger.info("copy log rotate config completed")
        # create symbolic link for this config file to be used by log rotation
        self.create_symbolic_link(self.get_confkey('S3_CONFIG_FILE').replace("/opt/seagate/cortx", self.base_config_file_path),
                                  self.get_confkey("S3_CONF_SYMLINK"))

      # update all the config files
      self.logger.info("update all services config files started")
      self.update_configs()
      self.logger.info("update all services config files completed")

      # validating config file after copying to /etc/cortx
      self.logger.info("validate config file started")
      self.validate_config_files(self.name)
      self.logger.info("validate config files completed")

      # read ldap credentials from config file
      self.logger.info("read ldap credentials started")
      self.read_ldap_credentials()
      self.read_ldap_root_credentials()
      self.logger.info("read ldap credentials completed")

      self.logger.info('create auth jks password started')
      self.create_auth_jks_password()
      self.logger.info('create auth jks password completed')


      if (self.services is not None) and (not 'openldap' in self.services):
        sysconfig_path = os.path.join(self.base_config_file_path,"s3","sysconfig",self.machine_id)
        file_name = sysconfig_path + '/s3server-0x*'
        list_matching = []
        for name in glob.glob(file_name):
          list_matching.append(name)
        count = len(list_matching)
        self.logger.info(f"s3server FID file count : {count}")
        s3_instance_count = int(self.get_confvalue_with_defaults('CONFIG>CONFSTORE_S3INSTANCES_KEY'))
        self.logger.info(f"s3_instance_count : {s3_instance_count}")
        if count < s3_instance_count:
          raise Exception("HARE-sysconfig file count does not match s3 instance count")
        index = 1
        for src_path in list_matching:
          file_name = 's3server-' + str(index)
          dst_path = os.path.join(sysconfig_path, file_name)
          self.create_symbolic_link(src_path, dst_path)
          index += 1

      # Configure s3 openldap schema
      self.push_s3_ldap_schema()

      if skip_haproxy == False:
        # Configure haproxy only
        self.configure_haproxy()

      # create topic for background delete
      bgdeleteconfig = CORTXS3Config(self.base_config_file_path, "yaml://")
      if bgdeleteconfig.get_messaging_platform() == MESSAGE_BUS:
        self.logger.info('Create topic started')
        self.create_topic(bgdeleteconfig.get_msgbus_admin_id,
                          bgdeleteconfig.get_msgbus_topic(),
                          self.get_msgbus_partition_count())
        self.logger.info('Create topic completed')

      # create background delete account
      ldap_endpoint_fqdn = self.get_endpoint("CONFIG>CONFSTORE_S3_OPENLDAP_ENDPOINTS", "fqdn", "ldap")

      self.logger.info("create background delete account started")
      self.create_bgdelete_account(ldap_endpoint_fqdn)
      self.logger.info("create background delete account completed")
      self.logger.info("making old config files for upgrade - started")
      # TODO : Based on service, call only necesssay files.
      self.make_sample_old_files([self.get_confkey('S3_CONFIG_SAMPLE_FILE'),
                    self.get_confkey('S3_AUTHSERVER_CONFIG_SAMPLE_FILE'),
                    self.get_confkey('S3_KEYSTORE_CONFIG_SAMPLE_FILE'),
                    self.get_confkey('S3_BGDELETE_CONFIG_SAMPLE_FILE'),
                    self.get_confkey('S3_CLUSTER_CONFIG_SAMPLE_FILE')])
      self.logger.info("making old config files for upgrade - complete")
    except Exception as e:
      raise S3PROVError(f'process() failed with exception: {e}')

  def create_symbolic_link(self, src_path: str, dst_path: str):
    """create symbolic link."""
    self.logger.info(f"symbolic link source path: {src_path}")
    self.logger.info(f"symbolic link destination path: {dst_path}")
    if os.path.exists(dst_path):
      self.logger.info(f"symbolic link is already present")
      os.unlink(dst_path)
      self.logger.info("symbolic link is unlinked")
    os.symlink(src_path, dst_path)
    self.logger.info(f"symbolic link created successfully")

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

  def push_s3_ldap_schema(self):
      """ Push s3 ldap schema with below checks,
          1. While pushing schema, global lock created in consul kv store as <index, key, value>.
             e.g. <s3_consul_index, component>s3>openldap_lock, machine_id>
          2. Before pushing s3 schema,
             a. Check for s3 openldap lock from consul kv store
             b. if lock is None/machine-id, then go ahead and push s3 ldap schema.
             c. if lock has other values except machine-id/None, then wait for the lock and retry again.
          3. Once s3 schema is pushed, delete the s3 key from consul kv store.
      """
      ldap_lock = False
      self.logger.info('checking for concurrent execution scenario for s3 ldap scheam push using consul kv lock.')
      openldap_key=self.get_confkey("S3_CONSUL_OPENLDAP_KEY")
      try:
          consul_endpoint_url=self.get_endpoint("CONFIG>CONFSTORE_CONSUL_ENDPOINTS", "fqdn", "http")
          consul_endpoint_port=self.get_endpoint("CONFIG>CONFSTORE_CONSUL_ENDPOINTS", "port", "http")
          consul_protocol='consul://'
          # consul url will be : consul://consul-server.default.svc.cluster.local:8500
          consul_url= f'{consul_protocol}'+ f'{consul_endpoint_url}' + ':' + f'{consul_endpoint_port}'
      except S3PROVError:
          # endpoint entry is not found in confstore hence fetch endpoint url from default value.
          consul_url=self.get_confvalue_with_defaults("DEFAULT_CONFIG>CONFSTORE_CONSUL_ENDPOINTS")
          self.logger.info(f'consul endpoint url entry (http://<consul-fqdn>:<port>) is missing for protocol type: http from confstore, hence using default value as {consul_url}')

      self.logger.info(f'loading consul service with consul endpoint URL as:{consul_url}')
      consul_confstore = S3CortxConfStore(config=f'{consul_url}', index=str(uuid.uuid1()))
      while(True):
          try:
              opendldap_val = consul_confstore.get_config(f'{openldap_key}')
              self.logger.info(f'openldap lock value is:{opendldap_val}')
              if opendldap_val is None:
                  self.logger.info(f'Setting confstore value for key :{openldap_key} and value as :{self.machine_id}')
                  consul_confstore.set_config(f'{openldap_key}', f'{self.machine_id}', True)
                  self.logger.info('Updated confstore with latest value')
                  time.sleep(5)
                  continue
              if opendldap_val == self.machine_id:
                  self.logger.info(f'Found lock acquired successfully hence processing with openldap schema push')
                  ldap_lock = True
                  break
              if opendldap_val != self.machine_id:
                  self.logger.info(f'openldap lock is already acquired by {opendldap_val}, Hence skipping openldap schema configuration')
                  ldap_lock = False
                  break

          except Exception as e:
              self.logger.error(f'Exception occured while connecting consul service endpoint {e}')
              break
      if ldap_lock == True:
        # push openldap schema
        self.logger.info('Pushing s3 ldap schema ....!!')
        self.configure_s3_schema()
        self.logger.info('Pushed s3 ldap schema successfully....!!')
        self.logger.info(f'Deleting consule key :{openldap_key}')
        consul_confstore.delete_key(f'{openldap_key}', True)
        self.logger.info(f'deleted openldap key-value from consul using consul endpoint URL as:{consul_url}')


  def configure_s3_schema(self):
    self.logger.info('openldap s3 configuration started')
    server_nodes_list_key = self.get_confkey('CONFIG>CONFSTORE_S3_OPENLDAP_SERVERS')
    server_nodes_list = self.get_confvalue(server_nodes_list_key)
    if type(server_nodes_list) is str:
      # list is stored as string in the confstore file
      server_nodes_list = literal_eval(server_nodes_list)
    for node_machine_id in server_nodes_list:
        cmd = ['/opt/seagate/cortx/s3/install/ldap/s3_setup_ldap.sh',
                '--hostname',
                f'{node_machine_id}',
                '--ldapuser',
                f'{self.ldap_user}',
                '--ldapadminpasswd',
                f'{self.ldap_passwd}',
                '--rootdnpasswd',
                f'{self.rootdn_passwd}']
        handler = SimpleProcess(cmd)
        stdout, stderr, retcode = handler.run()
        self.logger.info(f'output of setup_ldap.sh: {stdout}')
        if retcode != 0:
          self.logger.error(f'error of setup_ldap.sh: {stderr} {node_machine_id}')
          raise S3PROVError(f"{cmd} failed with err: {stderr}, out: {stdout}, ret: {retcode}")
        else:
          self.logger.warning(f'warning of setup_ldap.sh: {stderr} {node_machine_id}')

  def create_topic(self, admin_id: str, topic_name:str, partitions: int):
    """create topic for background delete services."""
    try:
      if not S3CortxMsgBus.is_topic_exist(admin_id, topic_name):
          S3CortxMsgBus.create_topic(admin_id, [topic_name], partitions)
          self.logger.info("Topic Created")
      else:
          self.logger.info("Topic Already exists")
    except Exception as e:
      raise e

  def get_msgbus_partition_count_Ex(self):
    """get total server nodes which will act as partition count."""
    # Get storage set count to loop over to get all nodes
    storage_set_count = self.get_confvalue_with_defaults('CONFIG>CONFSTORE_STORAGE_SET_COUNT')
    self.logger.info(f"storage_set_count : {storage_set_count}")
    srv_io_node_count = 0
    index = 0
    while index < int(storage_set_count):
      # Get all server nodes
      server_nodes_list_key = self.get_confkey('CONFIG>CONFSTORE_STORAGE_SET_SERVER_NODES_KEY').replace("storage_set_count", str(index))
      self.logger.info(f"server_nodes_list_key : {server_nodes_list_key}")
      server_nodes_list = self.get_confvalue(server_nodes_list_key)
      for server_node_id in server_nodes_list:
        self.logger.info(f"server_node_id : {server_node_id}")
        server_node_type_key = self.get_confkey('CONFIG>CONFSTORE_NODE_TYPE').replace('node-id', server_node_id)
        self.logger.info(f"server_node_type_key : {server_node_type_key}")
        # Get the type of each server node
        server_node_type = self.get_confvalue(server_node_type_key)
        self.logger.info(f"server_node_type : {server_node_type}")
        if server_node_type == "storage_node":
          self.logger.info(f"Node type is storage_node")
          srv_io_node_count += 1
      index += 1

    self.logger.info(f"Server io node count : {srv_io_node_count}")

    # Partition count should be ( number of hosts * 2 )
    partition_count = srv_io_node_count * 2
    self.logger.info(f"Partition count : {partition_count}")
    return partition_count

  def get_msgbus_partition_count(self):
    """get total consumers (* 2) which will act as partition count."""
    consumer_count = 0
    search_values = self.search_confvalue("node", "services", "bg_consumer")
    consumer_count = len(search_values)
    self.logger.info(f"consumer_count : {consumer_count}")

    # Partition count should be ( number of consumer * 2 )
    partition_count = consumer_count * 2
    self.logger.info(f"Partition count : {partition_count}")
    return partition_count

  def configure_haproxy(self):
    """Configure haproxy service."""
    self.logger.info('haproxy configuration started')
    try:

      # Create main config file for haproxy.
      S3HaproxyConfig(self.url).process()

      if "K8" != str(self.get_confvalue_with_defaults('CONFIG>CONFSTORE_SETUP_TYPE')):
        # update the haproxy log rotate config file in /etc/logrotate.d/haproxy
        self.find_and_replace("/etc/logrotate.d/haproxy", "/var/log/cortx", self.base_log_file_path)
        self.find_and_replace("/etc/rsyslog.d/haproxy.conf", "/var/log/cortx", self.base_log_file_path)

        # reload haproxy service
        try:
          self.logger.info("Reloading haproxy service...")
          service_list = ["haproxy"]
          self.reload_services(service_list)
        except Exception as e:
          self.logger.error(f'Failed to reload haproxy service, error: {e}')
          raise e
        self.logger.info("Reloaded haproxy service...")
      self.logger.info("Successfully configured haproxy on the node.")
    except Exception as e:
      self.logger.error(f'Failed to configure haproxy for s3server, error: {e}')
      raise e
    self.logger.info('haproxy configuration completed')

  def create_auth_jks_password(self):
    """Create random password for auth jks keystore."""
    cmd = ['sh',
      '/opt/seagate/cortx/auth/scripts/create_auth_jks_password.sh', self.base_config_file_path]
    handler = SimpleProcess(cmd)
    stdout, stderr, retcode = handler.run()
    self.logger.info(f'output of create_auth_jks_password.sh: {stdout}')
    if retcode != 0:
      self.logger.error(f'error of create_auth_jks_password.sh: {stderr}')
      raise S3PROVError(f"{cmd} failed with err: {stderr}, out: {stdout}, ret: {retcode}")
    else:
      self.logger.warning(f'warning of create_auth_jks_password.sh: {stderr}')
      self.logger.info(' Successfully set auth JKS keystore password.')

  def create_bgdelete_account(self, ldap_endpoint_fqdn: str):
    """ create bgdelete account."""
    try:
      # Create background delete account
      bgdelete_acc_input_params_dict = self.get_config_param_for_BG_delete_account()
      ldap_host_url="ldap://"+ldap_endpoint_fqdn
      LdapAccountAction(self.ldap_user, self.ldap_passwd).create_account(bgdelete_acc_input_params_dict, ldap_host_url)
    except Exception as e:
      if "Already exists" not in str(e):
        self.logger.error(f'Failed to create backgrounddelete service account, error: {e}')
        raise(e)
      else:
        self.logger.warning("backgrounddelete service account already exist")

  def update_config_value(self, config_file_path : str,
                          config_file_type : str,
                          key_to_read : str,
                          key_to_update: str,
                          modifier_function = None,
                          additional_param = None):
    """Update provided config key and value to provided config file.
       Modifier function should have the signature func_name(confstore, value)."""

    # validate config file exist (example: configfile = /etc/cortx/s3/conf/s3config.yaml)
    configfile = self.get_confkey(config_file_path).replace("/opt/seagate/cortx", self.base_config_file_path)
    if path.isfile(f'{configfile}') == False:
      self.logger.error(f'{configfile} file is not present')
      raise S3PROVError(f'{configfile} file is not present')

    # load config file (example: s3configfileconfstore = confstore object to /etc/cortx/s3/conf/s3config.yaml)
    s3configfileconfstore = S3CortxConfStore(f'{config_file_type}://{configfile}', 'update_config_file_idx' + key_to_update)

    # get the value to be updated from provisioner config for given key
    # Fetchinng the incoming value from the provisioner config file
    # Which should be updated to key_to_update in s3 config file
    value_to_update = self.get_confvalue_with_defaults(key_to_read)

    if modifier_function is not None:
      self.logger.info(f'Modifier function name : {modifier_function.__name__}')
      value_to_update = modifier_function(value_to_update, additional_param)

    self.logger.info(f'Key to update: {key_to_read}')
    self.logger.info(f'Value to update: {value_to_update}')

    # set the config value in to config file (example: s3 config file key_to_update = value_to_update, and save)
    s3configfileconfstore.set_config(key_to_update, value_to_update, True)
    self.logger.info(f'Key {key_to_update} updated successfully in {configfile}')

  def update_configs(self):
    """Update all service configs."""
    self.update_s3_server_configs()
    self.update_s3_auth_configs()
    self.update_s3_bgdelete_configs()
    self.update_s3_cluster_configs()

  def update_s3_server_configs(self):
    """ Update s3 server configs."""
    self.logger.info("Update s3 server config file started")
    #self.update_config_value("S3_CONFIG_FILE", "yaml", "CONFIG>CONFSTORE_S3SERVER_PORT", "S3_SERVER_CONFIG>S3_SERVER_BIND_PORT")
    self.update_config_value("S3_CONFIG_FILE", "yaml", "CONFIG>CONFSTORE_S3_BGDEL_BIND_ADDR", "S3_SERVER_CONFIG>S3_SERVER_BGDELETE_BIND_ADDR")
    self.update_config_value("S3_CONFIG_FILE", "yaml", "CONFIG>CONFSTORE_S3_INTERNAL_ENDPOINTS", "S3_SERVER_CONFIG>S3_SERVER_BGDELETE_BIND_PORT",self.update_s3_bgdelete_bind_port)
    self.update_config_value("S3_CONFIG_FILE", "yaml", "CONFIG>CONFSTORE_S3_AUTHSERVER_IP_ADDRESS", "S3_AUTH_CONFIG>S3_AUTH_IP_ADDR")
    self.update_config_value("S3_CONFIG_FILE", "yaml", "CONFIG>CONFSTORE_S3_AUTHSERVER_PORT", "S3_AUTH_CONFIG>S3_AUTH_PORT")
    self.update_config_value("S3_CONFIG_FILE", "yaml", "CONFIG>CONFSTORE_S3_ENABLE_STATS", "S3_SERVER_CONFIG>S3_ENABLE_STATS")
    self.update_config_value("S3_CONFIG_FILE", "yaml", "CONFIG>CONFSTORE_S3_AUDIT_LOGGER", "S3_SERVER_CONFIG>S3_AUDIT_LOGGER_POLICY")
    self.update_config_value("S3_CONFIG_FILE", "yaml", "CONFIG>CONFSTORE_BASE_LOG_PATH", "S3_SERVER_CONFIG>S3_LOG_DIR", self.update_s3_log_dir_path)
    self.update_config_value("S3_CONFIG_FILE", "yaml", "CONFIG>CONFSTORE_BASE_LOG_PATH", "S3_SERVER_CONFIG>S3_DAEMON_WORKING_DIR", self.update_s3_daemon_working_dir)
    self.update_config_value("S3_CONFIG_FILE", "yaml", "CONFIG>CONFSTORE_S3_MOTR_MAX_UNITS_PER_REQUEST", "S3_MOTR_CONFIG>S3_MOTR_MAX_UNITS_PER_REQUEST", self.update_motr_max_unit_per_request)
    self.update_config_value("S3_CONFIG_FILE", "yaml", "CONFIG>CONFSTORE_S3_MOTR_MAX_START_TIMEOUT", "S3_MOTR_CONFIG>S3_MOTR_INIT_MAX_TIMEOUT")
    self.logger.info("Update s3 server config file completed")

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

  def update_s3_bgdelete_bind_port(self, value_to_update, additional_param):
    if isinstance(value_to_update, str):
      value_to_update = literal_eval(value_to_update)
    endpoint = self.get_endpoint_for_scheme(value_to_update, "http")
    if 'port' not in endpoint:
      raise S3PROVError(f"BG Delete endpoint {value_to_update} does not have port specified.")
    if ("K8" == str(self.get_confvalue_with_defaults('CONFIG>CONFSTORE_SETUP_TYPE'))) :
      return int(endpoint['port']) -1
    else :
      return int(endpoint['port'])

  def update_s3_log_dir_path(self, value_to_update, additional_param):
    """ Update s3 server log directory path."""
    s3_log_dir_path = os.path.join(value_to_update, "s3", self.machine_id)
    self.logger.info(f"s3_log_dir_path : {s3_log_dir_path}")
    return s3_log_dir_path

  def update_s3_daemon_working_dir(self, value_to_update, additional_param):
    """ Update s3 daemon working log directory."""
    s3_daemon_working_dir = os.path.join(value_to_update, "motr", self.machine_id)
    self.logger.info(f"s3_daemon_working_dir : {s3_daemon_working_dir}")
    return s3_daemon_working_dir

  def update_motr_max_unit_per_request(self, value_to_update, additional_param):
    """Update motr max unit per request."""
    if 2 <= int(value_to_update) <= 128:
      if math.log2(int(value_to_update)).is_integer():
        self.logger.info("motr_max_units_per_request is in valid range")
      else:
        raise S3PROVError("motr_max_units_per_request should be power of 2")
    else:
      raise S3PROVError("motr_max_units_per_request should be between 2 to 128")
    return int(value_to_update)

  def update_s3_auth_configs(self):
    """ Update s3 auth configs."""
    self.logger.info("Update s3 authserver config file started")
    self.update_config_value("S3_AUTHSERVER_CONFIG_FILE", "properties", "CONFIG>CONFSTORE_S3_AUTHSERVER_HTTP_PORT", "httpPort")
    self.update_config_value("S3_AUTHSERVER_CONFIG_FILE", "properties", "CONFIG>CONFSTORE_S3_AUTHSERVER_HTTPS_PORT", "httpsPort")
    self.update_config_value("S3_AUTHSERVER_CONFIG_FILE", "properties", "CONFIG>CONFSTORE_S3_OPENLDAP_ENDPOINTS", "ldapHost",self.update_auth_ldap_host)
    self.update_config_value("S3_AUTHSERVER_CONFIG_FILE", "properties", "CONFIG>CONFSTORE_S3_OPENLDAP_ENDPOINTS", "ldapPort",self.update_auth_ldap_nonssl_port)
    self.update_config_value("S3_AUTHSERVER_CONFIG_FILE", "properties", "CONFIG>CONFSTORE_S3_OPENLDAP_ENDPOINTS", "ldapSSLPort",self.update_auth_ldap_ssl_port)
    self.update_config_value("S3_AUTHSERVER_CONFIG_FILE", "properties", "CONFIG>CONFSTORE_S3_AUTHSERVER_DEFAULT_ENDPOINT", "defaultEndpoint")
    self.update_config_value("S3_AUTHSERVER_CONFIG_FILE", "properties", "CONFIG>CONFSTORE_S3_AUTHSERVER_IAM_AUDITLOG", "IAMAuditlog")
    self.update_config_value("S3_AUTHSERVER_CONFIG_FILE", "properties", "CONFIG>CONFSTORE_BASE_LOG_PATH", "logFilePath", self.update_auth_log_dir_path)
    self.update_config_value("S3_AUTHSERVER_CONFIG_FILE", "properties", "CONFIG>CONFSTORE_BASE_CONFIG_PATH", "logConfigFile", self.update_auth_log4j_config_file_path)
    self.update_auth_log4j_log_dir_path()
    self.update_config_value("S3_AUTHSERVER_CONFIG_FILE", "properties", "CONFIG>CONFSTORE_LDAPADMIN_USER_KEY", "ldapLoginDN", self.update_auth_ldap_login_dn)
    self.update_config_value("S3_AUTHSERVER_CONFIG_FILE", "properties", "CONFIG>CONFSTORE_LDAPADMIN_PASSWD_KEY", "ldapLoginPW")
    self.logger.info("Update s3 authserver config file completed")

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

  def update_auth_ldap_host (self, value_to_update, additional_param):
    if type(value_to_update) is str:
      value_to_update = literal_eval(value_to_update)
    endpoint = self.get_endpoint_for_scheme(value_to_update, "ldap")
    if endpoint is None:
      raise S3PROVError(f"OpenLDAP endpoint for scheme 'ldap' is not specified")
    return endpoint['fqdn']


  def update_auth_ldap_ssl_port(self, value_to_update, additional_param):
    if type(value_to_update) is str:
      value_to_update = literal_eval(value_to_update)
    endpoint = self.get_endpoint_for_scheme(value_to_update, "ssl")
    if endpoint is None:
      raise S3PROVError(f"SSL LDAP endpoint is not specified.")
    if 'port' not in endpoint:
      raise S3PROVError(f"SSL LDAP endpoint does not specify port number.")
    return endpoint['port']

  def update_auth_ldap_nonssl_port(self, value_to_update, additional_param):
    if type(value_to_update) is str:
      value_to_update = literal_eval(value_to_update)
    endpoint = self.get_endpoint_for_scheme(value_to_update, "ldap")
    if endpoint is None:
      raise S3PROVError(f"Non-SSL LDAP endpoint is not specified.")
    if 'port' not in endpoint:
      raise S3PROVError(f"Non-SSL LDAP endpoint does not specify port number.")
    return endpoint['port']

  def update_auth_log_dir_path(self, value_to_update, additional_param):
    """Update s3 auth log directory path in config file."""
    s3_auth_log_path = os.path.join(value_to_update, "auth", self.machine_id, "server")
    self.logger.info(f's3_auth_log_path: {s3_auth_log_path}')
    return s3_auth_log_path

  def update_auth_log4j_config_file_path(self, value_to_update, additional_param):
    """Update s3 auth log4j config path in config file."""
    s3_auth_log4j_log_path = self.get_confkey("S3_AUTHSERVER_LOG4J2_CONFIG_FILE").replace("/opt/seagate/cortx", self.base_config_file_path)
    self.logger.info(f's3_auth_log4j_log_path: {s3_auth_log4j_log_path}')
    return s3_auth_log4j_log_path

  def update_auth_log4j_log_dir_path(self):
    """Update s3 auth log directory path in log4j2 config file."""
    # validate config file exist
    log4j2_configfile = self.get_confkey("S3_AUTHSERVER_LOG4J2_CONFIG_FILE").replace("/opt/seagate/cortx", self.base_config_file_path)
    if path.isfile(f'{log4j2_configfile}') == False:
      self.logger.error(f'{log4j2_configfile} file is not present')
      raise S3PROVError(f'{log4j2_configfile} file is not present')
    # parse the log4j xml file
    log4j2_xmlTree = ET.parse(log4j2_configfile)
    # get the root node
    rootElement = log4j2_xmlTree.getroot()
    # find the node Properties/Property
    propertiesElement = rootElement.find("Properties")
    propertyElement = propertiesElement.find("Property")
    s3_auth_log_path = os.path.join(self.base_log_file_path, "auth", self.machine_id, "server")
    self.logger.info(f's3_auth_log_path: {s3_auth_log_path}')
    # update the path in to xml
    propertyElement.text = s3_auth_log_path
    # Write the modified xml file.
    log4j2_xmlTree.write(log4j2_configfile)
    self.logger.info(f'Updated s3 auth log directory path in log4j2 config file')
  
  def update_auth_ldap_login_dn(self, value_to_update, additional_param):
    """Update s3 auth ldap login DN in config file."""
    s3_auth_ldap_login_dn = "cn=" + str(value_to_update) + ",dc=seagate,dc=com"
    self.logger.info(f's3_auth_ldap_login_dn: {s3_auth_ldap_login_dn}')
    return s3_auth_ldap_login_dn

  def update_s3_bgdelete_configs(self):
    """ Update s3 bgdelete configs."""
    self.logger.info("Update s3 bgdelete config file started")
    self.update_config_value("S3_BGDELETE_CONFIG_FILE", "yaml", "CONFIG>CONFSTORE_S3_INTERNAL_ENDPOINTS", "cortx_s3>producer_endpoint",self.update_bgdelete_producer_endpoint)
    self.update_config_value("S3_BGDELETE_CONFIG_FILE", "yaml", "CONFIG>CONFSTORE_S3_BGDELETE_CONSUMER_ENDPOINT", "cortx_s3>consumer_endpoint", self.update_bgdelete_consumer_endpoint)
    self.update_config_value("S3_BGDELETE_CONFIG_FILE", "yaml", "CONFIG>CONFSTORE_S3_BGDELETE_SCHEDULER_SCHEDULE_INTERVAL", "cortx_s3>scheduler_schedule_interval")
    self.update_config_value("S3_BGDELETE_CONFIG_FILE", "yaml", "CONFIG>CONFSTORE_S3_BGDELETE_MAX_KEYS", "indexid>max_keys")
    self.update_config_value("S3_BGDELETE_CONFIG_FILE", "yaml", "CONFIG>CONFSTORE_BASE_LOG_PATH", "logconfig>processor_logger_directory", self.update_bgdelete_processor_log_dir)
    self.update_config_value("S3_BGDELETE_CONFIG_FILE", "yaml", "CONFIG>CONFSTORE_BASE_LOG_PATH", "logconfig>scheduler_logger_directory", self.update_bgdelete_scheduler_log_dir)
    self.update_config_value("S3_BGDELETE_CONFIG_FILE", "yaml", "CONFIG>CONFSTORE_BASE_LOG_PATH", "logconfig>scheduler_log_file", self.update_bgdelete_scheduler_log_file_path)
    self.update_config_value("S3_BGDELETE_CONFIG_FILE", "yaml", "CONFIG>CONFSTORE_BASE_LOG_PATH", "logconfig>processor_log_file", self.update_bgdelete_processor_log_file_path)
    self.logger.info("Update s3 bgdelete config file completed")

  def update_bgdelete_producer_endpoint(self, value_to_update, additional_param):
    if isinstance(value_to_update, str):
      value_to_update = literal_eval(value_to_update)
    endpoint = self.get_endpoint_for_scheme(value_to_update, "http")
    if endpoint is None:
      raise S3PROVError(f"BG Producer endpoint for scheme 'http' is not specified")
    return endpoint['scheme'] + "://" + endpoint['fqdn'] + ":" + endpoint['port']

  def update_bgdelete_consumer_endpoint(self, value_to_update, additional_param):
    if isinstance(value_to_update, str):
      value_to_update = literal_eval(value_to_update)
    endpoint = self.get_endpoint_for_scheme(value_to_update, "http")
    if endpoint is None:
      raise S3PROVError(f"BG Consumer endpoint for scheme 'http' is not specified")
    if ("K8" == str(self.get_confvalue_with_defaults('CONFIG>CONFSTORE_SETUP_TYPE'))) :
      endpoint['port'] = int(endpoint['port']) -1
    else :
      endpoint['port'] = int(endpoint['port'])
    return endpoint['scheme'] + "://" + endpoint['fqdn'] + ":" + str(endpoint['port'])

  # In producer we do not append machine ID to path but below two functtions are for future 
  def update_bgdelete_scheduler_log_dir(self, value_to_update, additional_param):
    """ Update s3 bgdelete Scheduler log dir path."""
    bgdelete_log_dir_path = os.path.join(value_to_update, "s3", "s3backgrounddelete")
    self.logger.info(f"update_bgdelete_scheduler_log_dir : {bgdelete_log_dir_path}")
    return bgdelete_log_dir_path

  def update_bgdelete_scheduler_log_file_path(self, value_to_update, additional_param):
    """ Update s3 bgdelete scheduler log dir path."""
    bgdelete_scheduler_log_file_path = os.path.join(value_to_update, "s3/s3backgrounddelete/object_recovery_scheduler.log")
    self.logger.info(f"update_bgdelete_scheduler_log_file_path : {bgdelete_scheduler_log_file_path}")
    return bgdelete_scheduler_log_file_path

  def update_bgdelete_processor_log_dir(self, value_to_update, additional_param):
    """ Update s3 bgdelete processor log dir path."""
    bgdelete_log_dir_path = os.path.join(value_to_update, "s3", self.machine_id, "s3backgrounddelete")
    self.logger.info(f"update_bgdelete_processor_log_dir : {bgdelete_log_dir_path}")
    return bgdelete_log_dir_path

  def update_bgdelete_processor_log_file_path(self, value_to_update, additional_param):
    """ Update s3 bgdelete processor log dir path."""
    bgdelete_processor_log_file_path = os.path.join(value_to_update, "s3", self.machine_id, "s3backgrounddelete/object_recovery_processor.log")
    self.logger.info(f"update_bgdelete_processor_log_file_path : {bgdelete_processor_log_file_path}")
    return bgdelete_processor_log_file_path

  def update_s3_cluster_configs(self):
    """ Update s3 cluster configs."""
    self.logger.info("Update s3 cluster config file started")
    self.update_config_value("S3_CLUSTER_CONFIG_FILE", "yaml", "CONFIG>CONFSTORE_CLUSTER_ID_KEY", "cluster_config>cluster_id")
    self.update_config_value("S3_CLUSTER_CONFIG_FILE", "yaml", "CONFIG>CONFSTORE_ROOTDN_USER_KEY", "cluster_config>rootdn_user")
    self.update_config_value("S3_CLUSTER_CONFIG_FILE", "yaml", "CONFIG>CONFSTORE_ROOTDN_PASSWD_KEY", "cluster_config>rootdn_pass")
    self.logger.info("Update s3 cluster config file completed")

  def copy_s3authserver_resources(self):
    """Copy config files from /opt/seagate/cortx/auth/resources  to /etc/cortx/auth/resources."""
    src_authserver_resource_dir= self.get_confkey("S3_AUTHSERVER_RESOURCES_DIR")
    dest_authserver_resource_dir= self.get_confkey("S3_AUTHSERVER_RESOURCES_DIR").replace("/opt/seagate/cortx", self.base_config_file_path)
    for item in os.listdir(src_authserver_resource_dir):
      source = os.path.join(src_authserver_resource_dir, item)
      destination = os.path.join(dest_authserver_resource_dir, item)
      if os.path.isdir(source):
        if os.path.exists(destination):
          shutil.rmtree(destination)
        shutil.copytree(source, destination)
      else:
          shutil.copy2(source, destination)

  def copy_logrotate_files(self):
    """Copy log rotation config files from install directory to cron directory."""
    # Copy log rotate config files to /etc/logrotate.d/
    config_files = [self.get_confkey('S3_LOGROTATE_AUDITLOG')]
    self.copy_logrotate_files_crond(config_files, "/etc/logrotate.d/")

    # Copy log rotate config files to /etc/cron.hourly/
    config_files = [self.get_confkey('S3_LOGROTATE_S3LOG'),
                    self.get_confkey('S3_LOGROTATE_M0TRACE'),
                    self.get_confkey('S3_LOGROTATE_ADDB')]
    self.copy_logrotate_files_crond(config_files, "/etc/cron.hourly/")

  def copy_logrotate_files_crond(self, config_files, dest_directory):
    """Copy log rotation config files from install directory to cron directory."""
    # Copy log rotation config files from install directory to cron directory
    for config_file in config_files:
      self.logger.info(f"Source config file: {config_file}")
      self.logger.info(f"Dest dir: {dest_directory}")
      os.makedirs(os.path.dirname(dest_directory), exist_ok=True)
      shutil.copy(config_file, dest_directory)
      self.logger.info("Config file copied successfully to cron directory")

  def find_and_replace(self, filename: str,
                       content_to_search: str,
                       content_to_replace: str):
    """find and replace the given string."""
    self.logger.info(f"content_to_search: {content_to_search}")
    self.logger.info(f"content_to_replace: {content_to_replace}")
    with open(filename) as f:
      newText=f.read().replace(content_to_search, content_to_replace)
    with open(filename, "w") as f:
      f.write(newText)
    self.logger.info(f"find and replace completed successfully for the file {filename}.")
