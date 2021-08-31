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
import errno
import shutil
import math
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

class ConfigCmd(SetupCmd):
  """Config Setup Cmd."""
  name = "config"

  def __init__(self, config: str, module: str = None):
    """Constructor."""
    try:
      super(ConfigCmd, self).__init__(config, module)
    except Exception as e:
      raise S3PROVError(f'exception: {e}')

  def process(self, configure_only_openldap = False, configure_only_haproxy = False):
    """Main processing function."""
    self.logger.info(f"Processing phase = {self.name}, config = {self.url}, module = {self.module}")
    self.logger.info("validations started")
    self.phase_prereqs_validate(self.name)
    self.phase_keys_validate(self.url, self.name)
    self.logger.info("validations completed")

    try:
      # first update all the config files then do rest of the configurations
      self.logger.info("update all modules config files started")
      self.update_configs()
      self.logger.info("update all modules config files completed")

      self.logger.info("copy config files started")
      self.copy_config_files()
      self.logger.info("copy config files completed")

      # validating config file after copying to /etc/cortx
      self.logger.info("validate config file started")
      self.validate_config_files(self.name)
      self.logger.info("validate config files completed")

      # read ldap credentials from config file
      self.logger.info("read ldap credentials started")
      self.read_ldap_credentials()
      self.logger.info("read ldap credentials completed")

      # disable S3server, S3authserver, haproxy, BG delete services on reboot as 
      # it will be managed by HA
      self.logger.info('Disable services on reboot started')
      services_list = ["haproxy", "s3backgroundproducer", "s3backgroundconsumer", "s3server@*", "s3authserver"]
      self.disable_services(services_list)
      self.logger.info('Disable services on reboot completed')


      self.logger.info('create auth jks password started')
      self.create_auth_jks_password()
      self.logger.info('create auth jks password completed')

      if configure_only_openldap == True:
        # Configure openldap only
        self.configure_openldap()
      elif configure_only_haproxy == True:
        # Configure haproxy only
        self.configure_haproxy()
      else:
        # Configure both openldap and haproxy
        self.configure_openldap()
        self.configure_haproxy()

      # create topic for background delete
      bgdeleteconfig = CORTXS3Config()
      if bgdeleteconfig.get_messaging_platform() == MESSAGE_BUS:
        self.logger.info('Create topic started')
        self.create_topic(bgdeleteconfig.get_msgbus_admin_id,
                          bgdeleteconfig.get_msgbus_topic(),
                          self.get_msgbus_partition_count())
        self.logger.info('Create topic completed')

      # create background delete account
      self.logger.info("create background delete account started")
      self.create_bgdelete_account()
      self.logger.info("create background delete account completed")
    except Exception as e:
      raise S3PROVError(f'process() failed with exception: {e}')

  def configure_openldap(self):
    """Install and Configure Openldap over Non-SSL."""
    # 1. Install and Configure Openldap over Non-SSL.
    # 2. Enable slapd logging in rsyslog config
    # 3. Set openldap-replication
    # 4. Check number of nodes in the cluster
    # TODO pass the base config file path to the script.
    self.logger.info('Open ldap configuration started')
    cmd = ['/opt/seagate/cortx/s3/install/ldap/setup_ldap.sh',
           '--ldapadminpasswd',
           f'{self.ldap_passwd}',
           '--rootdnpasswd',
           f'{self.rootdn_passwd}',
           '--baseconfigpath'
           f'{self.base_config_file_path}',
           '--forceclean',
           '--skipssl']
    handler = SimpleProcess(cmd)
    stdout, stderr, retcode = handler.run()
    self.logger.info(f'output of setup_ldap.sh: {stdout}')
    if retcode != 0:
      self.logger.error(f'error of setup_ldap.sh: {stderr}')
      raise S3PROVError(f"{cmd} failed with err: {stderr}, out: {stdout}, ret: {retcode}")
    else:
      self.logger.warning(f'warning of setup_ldap.sh: {stderr}')

    if os.path.isfile("/opt/seagate/cortx/s3/install/ldap/rsyslog.d/slapdlog.conf"):
      try:
        os.makedirs("/etc/rsyslog.d")
      except OSError as e:
        if e.errno != errno.EEXIST:
          raise S3PROVError(f"mkdir /etc/rsyslog.d failed with errno: {e.errno}, exception: {e}")
      shutil.copy('/opt/seagate/cortx/s3/install/ldap/rsyslog.d/slapdlog.conf',
                  '/etc/rsyslog.d/slapdlog.conf')

    # restart rsyslog service
    try:
      self.logger.info("Restarting rsyslog service...")
      service_list = ["rsyslog"]
      self.restart_services(service_list)
    except Exception as e:
      self.logger.error(f'Failed to restart rsyslog service, error: {e}')
      raise e
    self.logger.info("Restarted rsyslog service...")

    # set openldap-replication
    self.configure_openldap_replication()
    self.logger.info('Open ldap configuration completed')

  def configure_openldap_replication(self):
    """Configure openldap replication within a storage set."""
    self.logger.info('Cleaning up old Openldap replication configuration')
    # Delete ldap replication cofiguration
    self.delete_replication_config()
    self.logger.info('Open ldap replication configuration started')
    storage_set_count = self.get_confvalue(self.get_confkey(
        'CONFIG>CONFSTORE_STORAGE_SET_COUNT_KEY').replace("cluster-id", self.cluster_id))

    index = 0
    while index < int(storage_set_count):
      server_nodes_list = self.get_confkey(
        'CONFIG>CONFSTORE_STORAGE_SET_SERVER_NODES_KEY').replace("cluster-id", self.cluster_id).replace("storage-set-count", str(index))
      server_nodes_list = self.get_confvalue(server_nodes_list)
      if type(server_nodes_list) is str:
        # list is stored as string in the confstore file
        server_nodes_list = literal_eval(server_nodes_list)

      if len(server_nodes_list) > 1:
        self.logger.info(f'Setting ldap-replication for storage_set:{index}')

        Path(self.s3_tmp_dir).mkdir(parents=True, exist_ok=True)
        ldap_hosts_list_file = os.path.join(self.s3_tmp_dir, "ldap_hosts_list_file.txt")
        with open(ldap_hosts_list_file, "w") as f:
          for node_machine_id in server_nodes_list:
            private_fqdn = self.get_confvalue(f'server_node>{node_machine_id}>network>data>private_fqdn')
            f.write(f'{private_fqdn}\n')
            self.logger.info(f'output of ldap_hosts_list_file.txt: {private_fqdn}')

        cmd = ['/opt/seagate/cortx/s3/install/ldap/replication/setupReplicationScript.sh',
             '-h',
             ldap_hosts_list_file,
             '-p',
             f'{self.rootdn_passwd}']
        handler = SimpleProcess(cmd)
        stdout, stderr, retcode = handler.run()
        self.logger.info(f'output of setupReplicationScript.sh: {stdout}')
        os.remove(ldap_hosts_list_file)

        if retcode != 0:
          self.logger.error(f'error of setupReplicationScript.sh: {stderr}')
          raise S3PROVError(f"{cmd} failed with err: {stderr}, out: {stdout}, ret: {retcode}")
        else:
          self.logger.warning(f'warning of setupReplicationScript.sh: {stderr}')
      index += 1
    # TODO: set replication across storage-sets
    self.logger.info('Open ldap replication configuration completed')

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

  def get_msgbus_partition_count(self):
    """get total server nodes which will act as partition count."""
    storage_set_count = self.get_confvalue(self.get_confkey(
      'CONFIG>CONFSTORE_STORAGE_SET_COUNT_KEY').replace("cluster-id", self.cluster_id))
    srv_count=0
    index = 0
    while index < int(storage_set_count):
      server_nodes_list = self.get_confkey(
        'CONFIG>CONFSTORE_STORAGE_SET_SERVER_NODES_KEY').replace("cluster-id", self.cluster_id).replace("storage-set-count", str(index))
      server_nodes_list = self.get_confvalue(server_nodes_list)
      if type(server_nodes_list) is str:
        # list is stored as string in the confstore file
        server_nodes_list = literal_eval(server_nodes_list)

      srv_count += len(server_nodes_list)
      index += 1
    self.logger.info(f"Server node count : {srv_count}")
    # Partition count should be ( number of hosts * 2 )
    srv_count = srv_count * 2
    self.logger.info(f"Partition count : {srv_count}")
    return srv_count

  def configure_haproxy(self):
    """Configure haproxy service."""
    self.logger.info('haproxy configuration started')
    try:
      S3HaproxyConfig(self.url).process()
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
      '/opt/seagate/cortx/auth/scripts/create_auth_jks_password.sh']
    handler = SimpleProcess(cmd)
    stdout, stderr, retcode = handler.run()
    self.logger.info(f'output of create_auth_jks_password.sh: {stdout}')
    if retcode != 0:
      self.logger.error(f'error of create_auth_jks_password.sh: {stderr}')
      raise S3PROVError(f"{cmd} failed with err: {stderr}, out: {stdout}, ret: {retcode}")
    else:
      self.logger.warning(f'warning of create_auth_jks_password.sh: {stderr}')
      self.logger.info(' Successfully set auth JKS keystore password.')

  def create_bgdelete_account(self):
    """ create bgdelete account."""
    try:
      # Create background delete account
      bgdelete_acc_input_params_dict = self.get_config_param_for_BG_delete_account()
      LdapAccountAction(self.ldap_user, self.ldap_passwd).create_account(bgdelete_acc_input_params_dict)
    except Exception as e:
      if "Already exists" not in str(e):
        self.logger.error(f'Failed to create backgrounddelete service account, error: {e}')
        raise(e)
      else:
        self.logger.warning("backgrounddelete service account already exist")

  def update_s3_server_config(self):
    """Update s3 server config which required modification."""

    # validate config file exist
    s3configfile = self.get_confkey('S3_CONFIG_FILE')
    if path.isfile(f'{s3configfile}') == False:
      self.logger.error(f'{s3configfile} file is not present')
      raise S3PROVError(f'{s3configfile} file is not present')

    # load s3config file
    s3configfileconfstore = S3CortxConfStore(f'yaml://{s3configfile}', 'write_s3_motr_max_unit_idx')

    #update S3_MOTR_MAX_UNITS_PER_REQUEST in the s3config file based on VM/OVA/HW
    #S3_MOTR_MAX_UNITS_PER_REQUEST = 8 for VM/OVA
    #S3_MOTR_MAX_UNITS_PER_REQUEST = 32 for HW
    # get the motr_max_units_per_request count from the config file
    motr_max_units_per_request = self.get_confvalue(self.get_confkey('CONFIG>CONFSTORE_S3_MOTR_MAX_UNITS_PER_REQUEST'))
    self.logger.info(f'motr_max_units_per_request: {motr_max_units_per_request}')
    #validate min and max unit should be between 1 to 128
    if 2 <= int(motr_max_units_per_request) <= 128:
      if math.log2(int(motr_max_units_per_request)).is_integer():
        self.logger.info("motr_max_units_per_request is in valid range")
      else:
        raise S3PROVError("motr_max_units_per_request should be power of 2")
    else:
      raise S3PROVError("motr_max_units_per_request should be between 2 to 128")

    # update the S3_MOTR_MAX_UNITS_PER_REQUEST in s3config.yaml file
    motr_max_units_per_request_key = 'S3_MOTR_CONFIG>S3_MOTR_MAX_UNITS_PER_REQUEST'
    s3configfileconfstore.set_config(motr_max_units_per_request_key, int(motr_max_units_per_request), True)
    self.logger.info(f'Key {motr_max_units_per_request_key} updated successfully in {s3configfile}')

    # update log path
    s3_log_path = self.get_confvalue(self.get_confkey('CONFIG>CONFSTORE_BASE_LOG_PATH'))
    s3_log_path = os.path.join(s3_log_path, "s3")
    self.logger.info(f's3_log_path: {s3_log_path}')
    s3_log_path_key  = 'S3_SERVER_CONFIG>S3_LOG_DIR'
    s3configfileconfstore.set_config(s3_log_path_key, s3_log_path, True)
    self.logger.info(f'Key {s3_log_path_key} updated successfully in {s3configfile}')


  def update_config_value(self, config_file_path : str,
                          config_file_type : str,
                          key_to_read : str,
                          key_to_update: str):
    """Update provided config key and value to provided config file."""

    # validate config file exist
    configfile = self.get_confkey(config_file_path)
    if path.isfile(f'{configfile}') == False:
      self.logger.error(f'{configfile} file is not present')
      raise S3PROVError(f'{configfile} file is not present')

    # load config file
    s3configfileconfstore = S3CortxConfStore(f'{config_file_type}://{configfile}', 'update_config_file_idx' + key_to_update)
    
    # get the value to be updated from provisioner config for given key
    value_to_update = self.get_confvalue(self.get_confkey(key_to_read))
    self.logger.info(f'{key_to_read}: {value_to_update}')

    # set the config value in to config file
    s3configfileconfstore.set_config(key_to_update, value_to_update, True)
    self.logger.info(f'Key {key_to_update} updated successfully in {configfile}')

  def update_configs(self):
    """Update all module configs."""
    self.update_s3_server_configs()
    self.update_s3_auth_configs()
    self.update_s3_bgdelete_configs()

  def update_s3_server_configs(self):
    """ Update s3 server configs."""
    self.logger.info("Update s3 server config file started")
    self.update_s3_server_config()
    self.update_config_value("S3_CONFIG_FILE", "yaml", "CONFIG>CONFSTORE_S3SERVER_IP_ADDRESS", "S3_SERVER_CONFIG>S3_SERVER_IPV4_BIND_ADDR")
    self.update_config_value("S3_CONFIG_FILE", "yaml", "CONFIG>CONFSTORE_S3SERVER_PORT", "S3_SERVER_CONFIG>S3_SERVER_BIND_PORT")
    self.update_config_value("S3_CONFIG_FILE", "yaml", "CONFIG>CONFSTORE_S3_SERVER_BGDELETE_BIND_ADDR", "S3_SERVER_CONFIG>S3_SERVER_BGDELETE_BIND_ADDR")
    self.update_config_value("S3_CONFIG_FILE", "yaml", "CONFIG>CONFSTORE_S3_SERVER_BGDELETE_BIND_PORT", "S3_SERVER_CONFIG>S3_SERVER_BGDELETE_BIND_PORT")
    self.update_config_value("S3_CONFIG_FILE", "yaml", "CONFIG>CONFSTORE_S3_AUTHSERVER_IP_ADDRESS", "S3_AUTH_CONFIG>S3_AUTH_IP_ADDR")
    self.update_config_value("S3_CONFIG_FILE", "yaml", "CONFIG>CONFSTORE_S3_AUTHSERVER_PORT", "S3_AUTH_CONFIG>S3_AUTH_PORT")
    self.update_config_value("S3_CONFIG_FILE", "yaml", "CONFIG>CONFSTORE_S3_ENABLE_STATS", "S3_SERVER_CONFIG>S3_ENABLE_STATS")
    self.update_config_value("S3_CONFIG_FILE", "yaml", "CONFIG>CONFSTORE_S3_AUDIT_LOGGER", "S3_SERVER_CONFIG>S3_AUDIT_LOGGER_POLICY")
    self.logger.info("Update s3 server config file completed")

  def update_s3_auth_configs(self):
    """ Update s3 auth configs."""
    self.logger.info("Update s3 authserver config file started")
    self.update_s3_auth_config()
    self.update_config_value("S3_AUTHSERVER_CONFIG_FILE", "properties", "CONFIG>CONFSTORE_S3_AUTHSERVER_DEFAULT_HOST", "defaultHost")
    self.update_config_value("S3_AUTHSERVER_CONFIG_FILE", "properties", "CONFIG>CONFSTORE_S3_AUTHSERVER_HTTP_PORT", "httpPort")
    self.update_config_value("S3_AUTHSERVER_CONFIG_FILE", "properties", "CONFIG>CONFSTORE_S3_AUTHSERVER_HTTPS_PORT", "httpsPort")
    self.update_config_value("S3_AUTHSERVER_CONFIG_FILE", "properties", "CONFIG>CONFSTORE_S3_AUTHSERVER_LDAP_HOST", "ldapHost")
    self.update_config_value("S3_AUTHSERVER_CONFIG_FILE", "properties", "CONFIG>CONFSTORE_S3_AUTHSERVER_LDAP_PORT", "ldapPort")
    self.update_config_value("S3_AUTHSERVER_CONFIG_FILE", "properties", "CONFIG>CONFSTORE_S3_AUTHSERVER_LDAP_SSL_PORT", "ldapSSLPort")
    self.update_config_value("S3_AUTHSERVER_CONFIG_FILE", "properties", "CONFIG>CONFSTORE_S3_AUTHSERVER_DEFAULT_ENDPOINT", "defaultEndpoint")
    self.update_config_value("S3_AUTHSERVER_CONFIG_FILE", "properties", "CONFIG>CONFSTORE_S3_AUTHSERVER_S3_ENDPOINT", "s3Endpoints")
    self.update_config_value("S3_AUTHSERVER_CONFIG_FILE", "properties", "CONFIG>CONFSTORE_S3_AUTHSERVER_IAM_AUDITLOG", "IAMAuditlog")
    self.logger.info("Update s3 authserver config file completed")

  def update_s3_auth_config(self):
    """Update s3 auth config which required modification."""

    s3auth_configfile = self.get_confkey('S3_AUTHSERVER_CONFIG_FILE')
    if path.isfile(f'{s3auth_configfile}') == False:
      self.logger.error(f'{s3auth_configfile} file is not present')
      raise S3PROVError(f'{s3auth_configfile} file is not present')

    # load s3 auth config file 
    s3configfileconfstore = S3CortxConfStore(f'properties://{s3auth_configfile}', 'update_s3_auth_config')

    s3_auth_base_log_path = self.get_confvalue(self.get_confkey('CONFIG>CONFSTORE_BASE_LOG_PATH'))
    # update log path
    s3_auth_log_path = os.path.join(s3_auth_base_log_path, "auth/server")
    self.logger.info(f's3_auth_log_path: {s3_auth_log_path}')
    s3_auth_log_path_key  = 'logFilePath'
    s3configfileconfstore.set_config(s3_auth_log_path_key, s3_auth_log_path, True)
    self.logger.info(f'Key {s3_auth_log_path_key} updated successfully in {s3auth_configfile}')

  def update_s3_bgdelete_configs(self):
    """ Update s3 bgdelete configs."""
    self.logger.info("Update s3 bgdelete config file started")
    self.update_cluster_id(self.get_confkey('S3_CLUSTER_CONFIG_FILE'))
    self.update_rootdn_credentials()
    self.update_s3_bgdelete_config()
    self.update_config_value("S3_BGDELETE_CONFIG_FILE", "yaml", "CONFIG>CONFSTORE_S3_BGDELETE_SCHEDULER_SCHEDULE_INTERVAL", "cortx_s3>scheduler_schedule_interval")
    self.update_config_value("S3_BGDELETE_CONFIG_FILE", "yaml", "CONFIG>CONFSTORE_S3_BGDELETE_MAX_KEYS", "indexid>max_keys")
    self.logger.info("Update s3 bgdelete config file completed")

  def update_s3_bgdelete_config(self):
    """ Update s3 bgdelete config which required modification."""

    # update bgdelete endpoints in to config file
    bgdelete_configfile = self.get_confkey('S3_BGDELETE_CONFIG_FILE')
    if path.isfile(f'{bgdelete_configfile}') == False:
      self.logger.error(f'{bgdelete_configfile} file is not present')
      raise S3PROVError(f'{bgdelete_configfile} file is not present')

    # load bgdelete config file 
    s3configfileconfstore = S3CortxConfStore(f'yaml://{bgdelete_configfile}', 'update_bgdelete_endpoint')

    # get the bgdelete endpoints from the config file
    bgdelete_endpoint = self.get_confvalue(self.get_confkey('CONFIG>CONFSTORE_S3_BGDELETE_ENDPOINT'))
    bgdelete_port = self.get_confvalue(self.get_confkey('CONFIG>CONFSTORE_S3_BGDELETE_PORT'))
    bgdelete_url = "http://" + bgdelete_endpoint + ":" + bgdelete_port
    self.logger.info(f'bgdelete_url: {bgdelete_url}')

    # update bgdelete endpoint
    endpoint_key = 'cortx_s3>endpoint'
    s3configfileconfstore.set_config(endpoint_key, bgdelete_url, True)
    self.logger.info(f'Key {endpoint_key} updated successfully in {bgdelete_configfile}')

    s3_bgdelete_base_log_path = self.get_confvalue(self.get_confkey('CONFIG>CONFSTORE_BASE_LOG_PATH'))
    # update log path
    s3_bgdelete_log_path = os.path.join(s3_bgdelete_base_log_path, "s3/s3backgrounddelete")
    self.logger.info(f's3_bgdelete_log_path: {s3_bgdelete_log_path}')
    s3_bgdelete_log_path_key  = 'logconfig>logger_directory'
    s3configfileconfstore.set_config(s3_bgdelete_log_path_key, s3_bgdelete_log_path, True)
    self.logger.info(f'Key {s3_bgdelete_log_path_key} updated successfully in {bgdelete_configfile}')

    # update producer log path
    s3_bgdelete_producer_log_path = os.path.join(s3_bgdelete_base_log_path, "s3/s3backgrounddelete/object_recovery_scheduler.log")
    self.logger.info(f's3_bgdelete_producer_log_path: {s3_bgdelete_producer_log_path}')
    s3_bgdelete_producer_log_path_key  = 'logconfig>scheduler_log_file'
    s3configfileconfstore.set_config(s3_bgdelete_producer_log_path_key, s3_bgdelete_producer_log_path, True)
    self.logger.info(f'Key {s3_bgdelete_producer_log_path_key} updated successfully in {bgdelete_configfile}')

    # update consumer log path
    s3_bgdelete_consumer_log_path = os.path.join(s3_bgdelete_base_log_path, "s3/s3backgrounddelete/object_recovery_processor.log")
    self.logger.info(f's3_bgdelete_consumer_log_path: {s3_bgdelete_consumer_log_path}')
    s3_bgdelete_consumer_log_path_key  = 'logconfig>processor_log_file'
    s3configfileconfstore.set_config(s3_bgdelete_consumer_log_path_key, s3_bgdelete_consumer_log_path, True)
    self.logger.info(f'Key {s3_bgdelete_consumer_log_path_key} updated successfully in {bgdelete_configfile}')

  def update_config_path_files(self, file_to_search: str, key_to_search: str, key_to_replace: str):
    """ update the config file path in the files"""
    try:
      with open(file_to_search) as f:
        data=f.read().replace(key_to_search, key_to_replace)

      with open(file_to_search, "w") as f:
        f.write(data)
    except:
      self.logger.error(f'Failed to update config file path {file_to_search}')
      raise S3PROVError(f'Failed to update config file path {file_to_search}')

  def copy_config_files(self):
    """ Copy config files from /opt/seagate/cortx to /etc/cortx."""
    config_files = [self.get_confkey('S3_CONFIG_FILE'),
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
                    self.get_confkey('S3_CLUSTER_CONFIG_UNSAFE_ATTR_FILE')]

    # copy all the config files from the /opt/seagate/cortx to /etc/cortx
    for config_file in config_files:
      self.logger.info(f"Source config file: {config_file}")
      dest_config_file = config_file.replace("/opt/seagate/cortx", self.base_config_file_path)
      self.logger.info(f"Dest config file: {dest_config_file}")
      os.makedirs(os.path.dirname(dest_config_file), exist_ok=True)
      shutil.move(config_file, dest_config_file)
      self.logger.info("Config file copied successfully to /etc/cortx")