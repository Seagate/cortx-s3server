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

#!/usr/bin/python3.6

import mock
import unittest
import json
import os.path
from s3confstore.cortx_s3_confstore import S3CortxConfStore
from cortx.utils.conf_store import Conf
import tempfile

class S3ConfStoreAPIsUT(unittest.TestCase):
  """
  UT to test current APIs"""

  def test_json_conf(self):
    index = "dummy_idx_1"
    s3confstore = S3CortxConfStore()
    test_config = {
      'cluster': {
        "cluster_id": 'abcd-efgh-ijkl-mnop',
        "cluster_hosts": 'myhost-1,myhost2,myhost-3',
      }
    }
    tmpdir = tempfile.mkdtemp()
    filename = 'cortx_s3_confstoreuttest.json'
    saved_umask = os.umask(0o077)
    path = os.path.join(tmpdir, filename)
    with open(path, 'w+') as file:
      json.dump(test_config, file, indent=2)
    conf_url='json://' + path
    s3confstore.load_config(index, conf_url)
    result_data = s3confstore.get_config(index, 'cluster')
    if 'cluster_id' not in result_data:
      os.remove(path)
      os.umask(saved_umask)
      os.rmdir(tmpdir)
      self.assertFalse(True)
    s3confstore.set_config(index, 'cluster>cluster_id', '1234', False)
    result_data = s3confstore.get_config(index, 'cluster>cluster_id')
    if result_data != '1234':
      os.remove(path)
      os.umask(saved_umask)
      os.rmdir(tmpdir)
      self.assertFalse(True)
    os.remove(path)
    os.umask(saved_umask)
    os.rmdir(tmpdir)

  def test_yaml_conf(self):
    index = "dummy_idx_2"
    s3confstore = S3CortxConfStore()
    test_config = "bridge: {manufacturer: homebridge.io, model: homebridge, name: Homebridge, pin: 031-45-154, port: '51840', username: 'CC:22:3D:E3:CE:30'}"
    tmpdir = tempfile.mkdtemp()
    filename = 'cortx_s3_confstoreuttest.yaml'
    saved_umask = os.umask(0o077)
    path = os.path.join(tmpdir, filename)
    with open(path, 'w+') as file:
      file.write(test_config)
    conf_url='yaml://' + path
    s3confstore.load_config(index, conf_url)
    result_data = s3confstore.get_config(index, 'bridge')
    if 'port' not in result_data:
      os.remove(path)
      os.umask(saved_umask)
      os.rmdir(tmpdir)
      self.assertFalse(True)
    s3confstore.set_config(index, 'bridge>port', '1234', False)
    result_data = s3confstore.get_config(index, 'bridge>port')
    if result_data != '1234':
      os.remove(path)
      os.umask(saved_umask)
      os.rmdir(tmpdir)
      self.assertFalse(True)
    os.remove(path)
    os.umask(saved_umask)
    os.rmdir(tmpdir)

  @mock.patch.object(Conf, 'load')
  @mock.patch.object(Conf, 'get')
  def test_mock_load(self, mock_load_return, mock_get_return):
    index = "dummy_idx_3"
    s3confstore = S3CortxConfStore()
    mock_load_return.return_value=None
    mock_get_return.return_value = None
    s3confstore.load_config(None, "dummy")
    result_data = s3confstore.get_config(index, 'bridge')
    self.assertTrue(result_data == mock_get_return.return_value)

  @mock.patch.object(Conf, 'get')
  def test_mock_get(self, mock_get_return):
    index = "dummy_idx_4"
    s3confstore = S3CortxConfStore()
    mock_get_return.return_value = "dummy_return: dummy_123"
    result_data = s3confstore.get_config(index, 'bridge')
    self.assertTrue('dummy_123' in result_data)

  @mock.patch.object(Conf, 'load')
  @mock.patch.object(Conf, 'get')
  @mock.patch.object(Conf, 'set')
  def test_mock_set(self, mock_load_return, mock_get_return, mock_set_return):
    index = "dummy_idx_5"
    s3confstore = S3CortxConfStore()
    mock_load_return.return_value=None
    mock_get_return.return_value = None
    mock_set_return.return_value = None
    s3confstore.load_config(None, "dummy")
    s3confstore.set_config(index, "dummykey", "bridge:NA", False)
    result_data = s3confstore.get_config(index, 'bridge')
    self.assertTrue(result_data == mock_get_return.return_value)

  @mock.patch.object(Conf, 'load')
  @mock.patch.object(Conf, 'get')
  @mock.patch.object(Conf, 'set')
  @mock.patch.object(Conf, 'save')
  def test_mock_save(self, mock_load_return, mock_get_return, mock_set_return, mock_save_return):
    index = "dummy_idx_6"
    s3confstore = S3CortxConfStore()
    mock_load_return.return_value=None
    mock_get_return.return_value = None
    mock_set_return.return_value = None
    mock_save_return.return_value = None
    s3confstore.load_config(None, "dummy")
    s3confstore.set_config(index, "dummykey", "bridge:NA", True)
    result_data = s3confstore.get_config(index, 'bridge')
    self.assertTrue(result_data == mock_get_return.return_value)
