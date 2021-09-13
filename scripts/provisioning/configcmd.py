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
from s3cipher.cortx_s3_cipher import CortxS3Cipher
import xml.etree.ElementTree as ET

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

      # copy config files from /opt/seagate to base dir of config files (/etc/cortx)
      self.logger.info("copy config files started")
      self.copy_config_files()
      self.logger.info("copy config files completed")

      self.logger.info("copy s3 authserver resources started")
      self.copy_s3authserver_resources()
      self.logger.info("copy s3 authserver resources completed")

      # update all the config files
      self.logger.info("update all modules config files started")
      self.update_configs()
      self.logger.info("update all modules config files completed")

      # validating config file after copying to /etc/cortx
      self.logger.info("validate config file started")
      self.validate_config_files(self.name)
      self.logger.info("validate config files completed")

      # read ldap credentials from config file
      self.logger.info("read ldap credentials started")
      self.read_ldap_credentials()
      self.read_ldap_root_credentials()
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
           '--baseconfigpath',
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
            private_fqdn = self.get_confvalue(self.get_confkey('CONFIG>CONFSTORE_PRIVATE_FQDN_KEY').replace('machine-id', node_machine_id))
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
    storage_set_count = self.get_confvalue_with_defaults('CONFIG>CONFSTORE_STORAGE_SET_COUNT_KEY')
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
    """Update all module configs."""
    self.update_s3_server_configs()
    self.update_s3_auth_configs()
    self.update_s3_bgdelete_configs()
    self.update_s3_cluster_configs()

  def update_s3_server_configs(self):
    """ Update s3 server configs."""
    self.logger.info("Update s3 server config file started")
    self.update_config_value("S3_CONFIG_FILE", "yaml", "CONFIG>CONFSTORE_S3SERVER_PORT", "S3_SERVER_CONFIG>S3_SERVER_BIND_PORT")
    self.update_config_value("S3_CONFIG_FILE", "yaml", "CONFIG>CONFSTORE_S3_SERVER_BGDELETE_BIND_PORT", "S3_SERVER_CONFIG>S3_SERVER_BGDELETE_BIND_PORT")
    self.update_config_value("S3_CONFIG_FILE", "yaml", "CONFIG>CONFSTORE_S3_AUTHSERVER_IP_ADDRESS", "S3_AUTH_CONFIG>S3_AUTH_IP_ADDR")
    self.update_config_value("S3_CONFIG_FILE", "yaml", "CONFIG>CONFSTORE_S3_AUTHSERVER_PORT", "S3_AUTH_CONFIG>S3_AUTH_PORT")
    self.update_config_value("S3_CONFIG_FILE", "yaml", "CONFIG>CONFSTORE_S3_ENABLE_STATS", "S3_SERVER_CONFIG>S3_ENABLE_STATS")
    self.update_config_value("S3_CONFIG_FILE", "yaml", "CONFIG>CONFSTORE_S3_AUDIT_LOGGER", "S3_SERVER_CONFIG>S3_AUDIT_LOGGER_POLICY")
    self.update_config_value("S3_CONFIG_FILE", "yaml", "CONFIG>CONFSTORE_BASE_LOG_PATH", "S3_SERVER_CONFIG>S3_LOG_DIR", self.update_s3_log_dir_path)
    self.update_config_value("S3_CONFIG_FILE", "yaml", "CONFIG>CONFSTORE_BASE_LOG_PATH", "S3_SERVER_CONFIG>S3_DAEMON_WORKING_DIR", self.update_s3_daemon_working_dir)
    self.update_config_value("S3_CONFIG_FILE", "yaml", "CONFIG>CONFSTORE_S3_MOTR_MAX_UNITS_PER_REQUEST", "S3_MOTR_CONFIG>S3_MOTR_MAX_UNITS_PER_REQUEST", self.update_motr_max_unit_per_request)
    self.logger.info("Update s3 server config file completed")

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
    self.update_config_value("S3_AUTHSERVER_CONFIG_FILE", "properties", "CONFIG>CONFSTORE_S3_AUTHSERVER_LDAP_HOST", "ldapHost")
    self.update_config_value("S3_AUTHSERVER_CONFIG_FILE", "properties", "CONFIG>CONFSTORE_S3_AUTHSERVER_LDAP_PORT", "ldapPort")
    self.update_config_value("S3_AUTHSERVER_CONFIG_FILE", "properties", "CONFIG>CONFSTORE_S3_AUTHSERVER_LDAP_SSL_PORT", "ldapSSLPort")
    self.update_config_value("S3_AUTHSERVER_CONFIG_FILE", "properties", "CONFIG>CONFSTORE_S3_AUTHSERVER_DEFAULT_ENDPOINT", "defaultEndpoint")
    self.update_config_value("S3_AUTHSERVER_CONFIG_FILE", "properties", "CONFIG>CONFSTORE_S3_AUTHSERVER_IAM_AUDITLOG", "IAMAuditlog")
    self.update_config_value("S3_AUTHSERVER_CONFIG_FILE", "properties", "CONFIG>CONFSTORE_BASE_LOG_PATH", "logFilePath", self.update_auth_log_dir_path)
    self.update_config_value("S3_AUTHSERVER_CONFIG_FILE", "properties", "CONFIG>CONFSTORE_BASE_CONFIG_PATH", "logConfigFile", self.update_auth_log4j_config_file_path)
    self.update_auth_log4j_log_dir_path()
    self.logger.info("Update s3 authserver config file completed")

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

  def update_s3_bgdelete_configs(self):
    """ Update s3 bgdelete configs."""
    self.logger.info("Update s3 bgdelete config file started")
    self.update_config_value("S3_BGDELETE_CONFIG_FILE", "yaml", "CONFIG>CONFSTORE_S3_BGDELETE_PRODUCER_ENDPOINT", "cortx_s3>producer_endpoint")
    self.update_config_value("S3_BGDELETE_CONFIG_FILE", "yaml", "CONFIG>CONFSTORE_S3_BGDELETE_CONSUMER_ENDPOINT", "cortx_s3>consumer_endpoint")
    self.update_config_value("S3_BGDELETE_CONFIG_FILE", "yaml", "CONFIG>CONFSTORE_S3_BGDELETE_SCHEDULER_SCHEDULE_INTERVAL", "cortx_s3>scheduler_schedule_interval")
    self.update_config_value("S3_BGDELETE_CONFIG_FILE", "yaml", "CONFIG>CONFSTORE_S3_BGDELETE_MAX_KEYS", "indexid>max_keys")
    self.update_config_value("S3_BGDELETE_CONFIG_FILE", "yaml", "CONFIG>CONFSTORE_BASE_LOG_PATH", "logconfig>logger_directory", self.update_bgdelete_log_dir)
    self.update_config_value("S3_BGDELETE_CONFIG_FILE", "yaml", "CONFIG>CONFSTORE_BASE_LOG_PATH", "logconfig>scheduler_log_file", self.update_bgdelete_scheduler_log_file_path)
    self.update_config_value("S3_BGDELETE_CONFIG_FILE", "yaml", "CONFIG>CONFSTORE_BASE_LOG_PATH", "logconfig>processor_log_file", self.update_bgdelete_processor_log_file_path)
    self.logger.info("Update s3 bgdelete config file completed")

  def update_bgdelete_log_dir(self, value_to_update, additional_param):
    """ Update s3 bgdelete log dir path."""
    bgdelete_log_dir_path = os.path.join(value_to_update, "s3", self.machine_id, "s3backgrounddelete")
    self.logger.info(f"bgdelete_log_dir_path : {bgdelete_log_dir_path}")
    return bgdelete_log_dir_path

  def update_bgdelete_scheduler_log_file_path(self, value_to_update, additional_param):
    """ Update s3 bgdelete scheduler log dir path."""
    bgdelete_scheduler_log_file_path = os.path.join(value_to_update, "s3/s3backgrounddelete/object_recovery_scheduler.log")
    self.logger.info(f"bgdelete_scheduler_log_file_path : {bgdelete_scheduler_log_file_path}")
    return bgdelete_scheduler_log_file_path

  def update_bgdelete_processor_log_file_path(self, value_to_update, additional_param):
    """ Update s3 bgdelete processor log dir path."""
    bgdelete_processor_log_file_path = os.path.join(value_to_update, "s3", self.machine_id, "s3backgrounddelete/object_recovery_processor.log")
    self.logger.info(f"bgdelete_processor_log_file_path : {bgdelete_processor_log_file_path}")
    return bgdelete_processor_log_file_path

  def update_s3_cluster_configs(self):
    """ Update s3 cluster configs."""
    self.logger.info("Update s3 cluster config file started")
    self.update_config_value("S3_CLUSTER_CONFIG_FILE", "yaml", "CONFIG>CONFSTORE_CLUSTER_ID_KEY", "cluster_config>cluster_id")
    self.update_config_value("S3_CLUSTER_CONFIG_FILE", "yaml", "CONFIG>CONFSTORE_ROOTDN_USER_KEY", "cluster_config>rootdn_user")
    self.update_config_value("S3_CLUSTER_CONFIG_FILE", "yaml", "CONFIG>CONFSTORE_ROOTDN_PASSWD_KEY", "cluster_config>rootdn_pass")
    self.logger.info("Update s3 cluster config file completed")

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
