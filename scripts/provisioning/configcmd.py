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
from setupcmd import SetupCmd, S3PROVError
from cortx.utils.process import SimpleProcess

class ConfigCmd(SetupCmd):
  """Config Setup Cmd."""
  name = "config"

  def __init__(self, config: str):
    """Constructor."""
    try:
      super(ConfigCmd, self).__init__(config)

      self.read_cluster_id()
      self.write_cluster_id()
      self.read_ldap_credentials()
      self.read_node_info()

    except Exception as e:
      raise S3PROVError(f'exception: {e}\n')

  def process(self):
    """Main processing function."""
    sys.stdout.write(f"Processing {self.name} {self.url}\n")
    self.phase_prereqs_validate(self.name)

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

    if self.server_nodes_count > 1:
      sys.stdout.write(f"INFO: Setting ldap-replication as the cluster node_count {self.server_nodes_count} is greater than 2.\n")
      with open("hosts_list_file.txt", "w") as f:
        for host in self.hosts_list:
          f.write(f"{host}\n")
      sys.stdout.write("setting ldap-replication on all cluster nodes..\n")
      cmd = ['/opt/seagate/cortx/s3/install/ldap/replication/setupReplicationScript.sh',
             '-h',
             'hosts_list_file.txt',
             '-p',
             f'{self.rootdn_passwd}']
      handler = SimpleProcess(cmd)
      stdout, stderr, retcode = handler.run()
      if retcode != 0:
        raise S3PROVError(f"{cmd} failed with err: {stderr}, out: {stdout}, ret: {retcode}\n")
      os.remove("hosts_list_file.txt")
    cmd = ['systemctl', 'restart', 'slapd']
    handler = SimpleProcess(cmd)
    stdout, stderr, retcode = handler.run()
    if retcode != 0:
      raise S3PROVError(f"{cmd} failed with err: {stderr}, out: {stdout}, ret: {retcode}\n")

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
