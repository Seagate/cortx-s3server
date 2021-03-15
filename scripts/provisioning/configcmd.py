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

from cortx.utils.conf_store.conf_store import Conf
from setupcmd import SetupCmd, S3PROVError
from cortx.utils.process import SimpleProcess

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

  def process(self):
    """Main processing function."""
    sys.stdout.write(f"Processing {self.name} {self.url}\n")

    try:
      # Configure openldap and ldap-replication
      self.configure_openldap()
      sys.stdout.write("INFO: Successfully configured openldap on the node.\n")
      # Configure haproxy
      self.configure_haproxy()
      sys.stdout.write("INFO: Successfully configured haproxy on the node.\n")
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

    cmd = ['systemctl', 'restart', 'rsyslog']
    handler = SimpleProcess(cmd)
    stdout, stderr, retcode = handler.run()
    if retcode != 0:
      raise S3PROVError(f"{cmd} failed with err: {stderr}, out: {stdout}, ret: {retcode}\n")

    # set openldap-replication
    self.configure_openldap_replication()  

    cmd = ['systemctl', 'restart', 'slapd']
    handler = SimpleProcess(cmd)
    stdout, stderr, retcode = handler.run()
    if retcode != 0:
      raise S3PROVError(f"{cmd} failed with err: {stderr}, out: {stdout}, ret: {retcode}\n")

  def configure_openldap_replication(self):
    """Configure openldap replication within a storage set."""
    storage_set_count = Conf.get('provstore',
                              Conf.get('localstore',
                              'CONFSTORE_STORAGE_SET_COUNT_KEY').format(self.cluster_id))
    index = 0
    while index < storage_set_count:
      server_nodes_list = Conf.get('provstore',
                              Conf.get('localstore',
                              'CONFSTORE_STORAGE_SET_SERVER_NODES_KEY').format(self.cluster_id, index))
      if len(server_nodes_list) > 1:
        sys.stdout.write(f'Setting ldap-replication for storage_set:{index}\n')

        with open("hosts_list_file.txt", "w") as f:
          for node_machine_id in server_nodes_list:
            hostname = Conf.get('provstore', f'server_node>{node_machine_id}>hostname')
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

  def configure_haproxy(self):
    """Configure haproxy service."""
    cmd = ['s3haproxyconfig',
           '--path',
           f'{self._url}']
    handler = SimpleProcess(cmd)
    stdout, stderr, retcode = handler.run()
    if retcode != 0:
      raise S3PROVError(f"{cmd} failed with err: {stderr}, out: {stdout}, ret: {retcode}\n")
    cmd = ['systemctl', 'restart', 'haproxy']
    handler = SimpleProcess(cmd)
    stdout, stderr, retcode = handler.run()
    if retcode != 0:
      raise S3PROVError(f"{cmd} failed with err: {stderr}, out: {stdout}, ret: {retcode}\n")
