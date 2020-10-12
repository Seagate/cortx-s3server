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

"""CORTXClusterConfig stores the configuration required for release cluster info."""

import sys
import os
import shutil
import yaml

class CipherInvalidToken(Exception):
    pass

class CORTXClusterConfig(object):

    """Configuration for s3 cluster info."""

    def __init__(self):
        """Load and initialise configuration."""
        self._load_and_fetch_config()

    @staticmethod
    def get_conf_dir():
        """Return configuration directory location."""
        return os.path.join(os.path.dirname(__file__), 'config')

    def _load_and_fetch_config(self):
        """Populate configuration data."""
        conf_home_dir = os.path.join(
            '/', 'opt', 'seagate', 'cortx', 's3', 's3backgrounddelete')
        self._conf_file = os.path.join(conf_home_dir, 's3_cluster.yaml')
        if not os.path.isfile(self._conf_file):
            try:
                os.stat(conf_home_dir)
            except BaseException:
                os.mkdir(conf_home_dir)
            shutil.copy(
                os.path.join(
                    self.get_conf_dir(),
                    's3_cluster.yaml'),
                self._conf_file)

        if not os.access(self._conf_file, os.R_OK):
            self.logger.error(
                "Failed to read " +
                self._conf_file +
                " it doesn't have read access")
            sys.exit()
        with open(self._conf_file, 'r') as file_config:
            self._config = yaml.safe_load(file_config)

    def get_cluster_id(self):
        """Return cluster_id from config file or KeyError."""
        if 'cluster_config' in self._config and self._config['cluster_config']['cluster_id']:
            return self._config['cluster_config']['cluster_id']
        else:
            raise KeyError("Could not parse cluster_id config file " + self._conf_file)
