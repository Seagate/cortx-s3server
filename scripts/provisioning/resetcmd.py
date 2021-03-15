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

from setupcmd import SetupCmd

class ResetCmd(SetupCmd):
  """Reset Setup Cmd."""
  name = "reset"

  def __init__(self, config: str):
    """Constructor."""
    try:
      super(ResetCmd, self).__init__(config)
    except Exception as e:
      raise e

  def process(self):
    """Main processing function."""
    sys.stdout.write(f"Processing {self.name} {self.url}\n")
    try:
      sys.stdout.write('INFO: Cleaning up log files.\n')
      self.CleanupLogs()
      sys.stdout.write('INFO:Log files cleanup successfull.\n')
    except Exception as e:
      sys.stderr.write(f'Failed to cleanup log directories or files, error: {e}\n')
      raise e


  def CleanupLogs(self):
    """Cleanup all the log directories and files."""
    # Backgrounddelete -> /var/log/seagate/s3/s3backgrounddelete/
    if os.path.exists("/var/log/seagate/s3/s3backgrounddelete"):
      self.DeleteDir("/var/log/seagate/s3/s3backgrounddelete/")

    #Audit -> /var/log/seagate/s3/audit
    if os.path.exists("/var/log/seagate/s3/audit"):
      self.DeleteDir("/var/log/seagate/s3/audit/")

    # s3 -> /var/log/seagate/s3/
    if os.path.exists("/var/log/seagate/s3"):
      self.DeleteDir("/var/log/seagate/s3/")

    #Auth -> /var/log/seagate/auth/
    if os.path.exists("/var/log/seagate/auth"):
      self.DeleteDir("/var/log/seagate/auth/")

    #S3 Motr -> /var/log/seagate/motr/
    if os.path.exists("/var/log/seagate/motr"):
      self.DeleteDir("/var/log/seagate/motr/")

    #HAproxy -> /var/log/haproxy.log
    #        -> /var/log/haproxy-status.log
    if os.path.exists("/var/log/haproxy.log"):
      os.remove("/var/log/haproxy.log")
    if os.path.exists("/var/log/haproxy-status.log"):
      os.remove("/var/log/haproxy-status.log")

    #Slapd -> /var/log/slapd.log
    if os.path.exists("/var/log/slapd.log"):
      os.remove("/var/log/slapd.log")

    #S3 Crash dumps -> /var/log/crash/core-s3server.*.gz
    if os.path.exists("/var/log/crash"):
      files = glob.glob('/var/log/crash/core-s3server.*.gz')
      for file in files:
        os.remove(file)

  def DeleteDir(self, dirname: str):
    """Delete files and directories inside given directory."""
    for filename in os.listdir(dirname):
      filepath = os.path.join(dirname, filename)
      try:
        if os.path.isfile(filepath):
          os.remove(filepath)
        elif os.path.isdir(filepath):
          shutil.rmtree(filepath)
      except Exception as e:
        sys.stderr.write(f'ERROR: Failed to delete: {filepath}, error: {str(e)}\n')
        raise e
