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
from  ast import literal_eval

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

    except Exception as e:
      raise S3PROVError(f'exception: {e}\n')

  def process(self, configure_only_openldap = False, configure_only_haproxy = False):
    """Main processing function."""
    sys.stdout.write(f"Processing {self.name} {self.url}\n")
    self.phase_prereqs_validate(self.name)
    self.phase_keys_validate(self.url, self.name)

    try:
      self.create_auth_jks_password()

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
        sys.stdout.write('INFO: Creating topic.\n')
        self.create_topic(bgdeleteconfig.get_msgbus_admin_id,
                          bgdeleteconfig.get_msgbus_topic(),
                          self.get_msgbus_partition_count())
        sys.stdout.write('INFO:Topic creation successful.\n')
    except Exception as e:
      raise S3PROVError(f'process() failed with exception: {e}\n')

  def configure_openldap(self):
    """Install and Configure Openldap over Non-SSL."""
    # 1. Install and Configure Openldap over Non-SSL.
    # 2. Enable slapd logging in rsyslog config
    # 3. Set openldap-replication
    # 4. Check number of nodes in the cluster
    cmd = ['/opt/seagate/cortx/s3/install/ldap/setup_ldap.sh',
           '--ldapadminpasswd',
           f'{self.ldap_passwd}',
           '--rootdnpasswd',
           f'{self.rootdn_passwd}',
           '--forceclean',
           '--skipssl']
    handler = SimpleProcess(cmd)
    stdout, stderr, retcode = handler.run()
    if retcode != 0:
      raise S3PROVError(f"{cmd} failed with err: {stderr}, out: {stdout}, ret: {retcode}\n")

    if os.path.isfile("/opt/seagate/cortx/s3/install/ldap/rsyslog.d/slapdlog.conf"):
      try:
        os.makedirs("/etc/rsyslog.d")
      except OSError as e:
        if e.errno != errno.EEXIST:
          raise S3PROVError(f"mkdir /etc/rsyslog.d failed with errno: {e.errno}, exception: {e}\n")
      shutil.copy('/opt/seagate/cortx/s3/install/ldap/rsyslog.d/slapdlog.conf',
                  '/etc/rsyslog.d/slapdlog.conf')

    # restart rsyslog service
    try:
      sys.stdout.write("Restarting rsyslog service...\n")
      service_list = ["rsyslog"]
      self.restart_services(service_list)
    except Exception as e:
      sys.stderr.write(f'Failed to restart rsyslog service, error: {e}\n')
      raise e
    sys.stdout.write("Restarted rsyslog service...\n")

    # set openldap-replication
    self.configure_openldap_replication()
    
    sys.stdout.write("INFO: Successfully configured openldap on the node.\n")

  def configure_openldap_replication(self):
    """Configure openldap replication within a storage set."""
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
        sys.stdout.write(f'\nSetting ldap-replication for storage_set:{index}\n\n')

        with open("hosts_list_file.txt", "w") as f:
          for node_machine_id in server_nodes_list:
            hostname = self.get_confvalue(f'server_node>{node_machine_id}>hostname')
            f.write(f'{hostname}\n')

        cmd = ['/opt/seagate/cortx/s3/install/ldap/replication/setupReplicationScript.sh',
             '-h',
             'hosts_list_file.txt',
             '-p',
             f'{self.rootdn_passwd}']
        handler = SimpleProcess(cmd)
        stdout, stderr, retcode = handler.run()

        os.remove("hosts_list_file.txt")

        if retcode != 0:
          raise S3PROVError(f"{cmd} failed with err: {stderr}, out: {stdout}, ret: {retcode}\n")
      index += 1
    # TODO: set replication across storage-sets

  def create_topic(self, admin_id: str, topic_name:str, partitions: int):
    """create topic for background delete services."""
    try:
      if not S3CortxMsgBus.is_topic_exist(admin_id, topic_name):
          S3CortxMsgBus.create_topic(admin_id, [topic_name], partitions)
          sys.stdout.write("Topic Created\n")
      else:
          sys.stdout.write("Topic Already exists\n")
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
    sys.stdout.write(f"Server node count : {srv_count}\n")
    # Partition count should be ( number of hosts * 2 )
    srv_count = srv_count * 2
    sys.stdout.write(f"Partition count : {srv_count}\n")
    return srv_count

  def configure_haproxy(self):
    """Configure haproxy service."""
    try:
      S3HaproxyConfig(self.url).process()
      # reload haproxy service
      try:
        sys.stdout.write("Reloading haproxy service...\n")
        service_list = ["haproxy"]
        self.reload_services(service_list)
      except Exception as e:
        sys.stderr.write(f'Failed to reload haproxy service, error: {e}\n')
        raise e
      sys.stdout.write("Reloaded haproxy service...\n")
      sys.stdout.write("INFO: Successfully configured haproxy on the node.\n")
    except Exception as e:
      sys.stderr.write(f'Failed to configure haproxy for s3server, error: {e}')
      raise e

  @staticmethod
  def create_auth_jks_password():
    """Create random password for auth jks keystore."""
    cmd = ['sh',
      '/opt/seagate/cortx/auth/scripts/create_auth_jks_password.sh']
    handler = SimpleProcess(cmd)
    stdout, stderr, retcode = handler.run()
    if retcode != 0:
      raise S3PROVError(f"{cmd} failed with err: {stderr}, out: {stdout}, ret: {retcode}\n")
    else:
      sys.stdout.write('INFO: Successfully set auth JKS keystore password.\n')
