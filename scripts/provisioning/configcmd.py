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

class ConfigCmd(SetupCmd):
  """Config Setup Cmd."""
  name = "config"

  def __init__(self, config: str):
    """Constructor."""
    try:
      super(ConfigCmd, self).__init__(config)

      self.update_cluster_id()
      self.read_ldap_credentials()
      self.update_rootdn_credentials()

    except Exception as e:
      raise S3PROVError(f'exception: {e}')

  def process(self, configure_only_openldap = False, configure_only_haproxy = False):
    """Main processing function."""
    self.logger.info(f"Processing {self.name} {self.url}")
    self.logger.info("validations started")
    self.phase_prereqs_validate(self.name)
    self.phase_keys_validate(self.url, self.name)
    self.validate_config_files(self.name)
    self.logger.info("validations completed")

    try:

      self.logger.info("update motr max units per request started")
      self.update_motr_max_units_per_request()
      self.logger.info("update motr max units per request completed")

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
        self.configure_s3_schema()
      elif configure_only_haproxy == True:
        # Configure haproxy only
        self.configure_haproxy()
      else:
        # Configure both openldap and haproxy
        self.configure_s3_schema()
        self.configure_haproxy()

      # create topic for background delete
      bgdeleteconfig = CORTXS3Config()
      if bgdeleteconfig.get_messaging_platform() == MESSAGE_BUS:
        self.logger.info('Create topic started')
        self.create_topic(bgdeleteconfig.get_msgbus_admin_id,
                          bgdeleteconfig.get_msgbus_topic(),
                          self.get_msgbus_partition_count())
        self.logger.info('Create topic completed')
    except Exception as e:
      raise S3PROVError(f'process() failed with exception: {e}')

  def configure_s3_schema(self):
    self.logger.info('openldap s3 configuration started')
    cmd = ['/opt/seagate/cortx/s3/install/ldap/s3_setup_ldap.sh',
           '--ldapadminpasswd',
           f'{self.ldap_passwd}',
           '--rootdnpasswd',
           f'{self.rootdn_passwd}',
           '--skipssl']
    handler = SimpleProcess(cmd)
    stdout, stderr, retcode = handler.run()
    self.logger.info(f'output of setup_ldap.sh: {stdout}')
    if retcode != 0:
      self.logger.error(f'error of setup_ldap.sh: {stderr}')
      raise S3PROVError(f"{cmd} failed with err: {stderr}, out: {stdout}, ret: {retcode}")
    else:
      self.logger.warning(f'warning of setup_ldap.sh: {stderr}')

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

  def update_motr_max_units_per_request(self):
    """
    update S3_MOTR_MAX_UNITS_PER_REQUEST in the s3config file based on VM/OVA/HW
    S3_MOTR_MAX_UNITS_PER_REQUEST = 8 for VM/OVA
    S3_MOTR_MAX_UNITS_PER_REQUEST = 32 for HW
    """

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
    s3configfile = self.get_confkey('S3_CONFIG_FILE')
    if path.isfile(f'{s3configfile}') == False:
      self.logger.error(f'{s3configfile} file is not present')
      raise S3PROVError(f'{s3configfile} file is not present')
    else:
      motr_max_units_per_request_key = 'S3_MOTR_CONFIG>S3_MOTR_MAX_UNITS_PER_REQUEST'
      s3configfileconfstore = S3CortxConfStore(f'yaml://{s3configfile}', 'write_s3_motr_max_unit_idx')
      s3configfileconfstore.set_config(motr_max_units_per_request_key, int(motr_max_units_per_request), True)
      self.logger.info(f'Key {motr_max_units_per_request_key} updated successfully in {s3configfile}')
