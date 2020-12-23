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
import argparse
# from cortx.utils.conf_store import Conf
from cortx.utils.conf_store import ConfStore
from urllib.parse import urlparse
import os.path
import json
import sys

class S3CortxConfStore:
  default_index = "default_cortx_s3_confstore_index"

  def __init__(self):
    """Instantiate constore"""
    self.conf_store = ConfStore()

  def load_config(self, index: str, backend_url: str):
    """Load Config into constore"""
    self.conf_store.load(index, backend_url)

  def get_config(self, index: str, key: str):
    """Get the key's config from constore"""
    result_get = self.conf_store.get(index, key, default_val=None)
    return result_get

  def get_data_config(self, index: str):
    """Obtains entire config for the given index from constore"""
    result_get_data = self.conf_store.get_data(index)
    return result_get_data

  def uttest(self, index: str):
    """This will be removed to ut directory later"""
    test_config = {
      'cluster': {
        "cluster_id": 'abcd-efgh-ijkl-mnop',
        "cluster_hosts": 'myhost-1,myhost2,myhost-3',
      }
    }
    with open(r'/tmp/cortx_s3_confstoreuttest.json', 'w+') as file:
      json.dump(test_config, file, indent=2)
    self.load_config(index, 'json:///tmp/cortx_s3_confstoreuttest.json')

    result_data = self.get_config(index, 'cluster')
    if 'cluster_id' not in result_data:
      print("get_config() failed!")
      os.remove("/tmp/cortx_s3_confstoreuttest.json")
      exit(-1)

    result_data = self.get_data_config(index)
    if 'cluster' not in result_data:
      print("get_data_config() failed!")
      os.remove("/tmp/cortx_s3_confstoreuttest.json")
      exit(-1)

    os.remove("/tmp/cortx_s3_confstoreuttest.json")
    print("cortx_s3_confstore unit test passed!")

  def run(self):
    parser = argparse.ArgumentParser(description='Cortx-Utils ConfStore')
    parser.add_argument("--load", help='Load the backing storage in this index')
    parser.add_argument("--get", help='Obtain config for this given index using a key, pass --key')
    parser.add_argument("--get_data", help='Obtains entire config for this given index')
    parser.add_argument("--key", help='Provide a key to be used in --get')
    parser.add_argument("--getkey", help='Fetch value of the given key from default index, pass --path to config')
    parser.add_argument("--path", help='example: JSON:///etc/cortx/myconf.json')
    parser.add_argument("--uttest", help='An unit test for the load, get and get_data APIs')

    args = parser.parse_args()

    if len(sys.argv) < 2:
      print("Incorrect args, use -h to review usage")
      exit(-1)

    if args.path:
      if os.path.isfile(urlparse(args.path).path) != True:
        print("config file: {} does not exist".format(args.path))
        exit(-1)
      else:
        try:
          with open(urlparse(args.path).path) as f:
            json.load(f)
        except ValueError as e:
          print("config file: {} must use valid JSON format: {}".format(urlparse(args.path).path, e))
          exit(-1)

    if args.get:
      if args.key == None:
        print("Please provide the key to be get from confstore using --key")
        exit(-1)
      result_get = self.get_config(args.get, args.key)
      if result_get:
        print("{}".format(result_get))

    if args.get_data:
      result_get_data = self.get_data_config(args.get_data)
      if result_get_data:
        print("{}".format(result_get_data))

    if args.load or args.getkey:
      if args.path == None:
        print("Please provide the path to be load into confstore")
        exit(-1)
      urlpath = args.path
      if args.load:
        self.load_config(args.load, urlpath)
        print("load conf: {} into index: {}".format(urlpath, args.load))
      elif args.getkey:
        self.load_config(self.default_index, urlpath)
        result_get = self.get_config(self.default_index, args.getkey)
        if result_get:
          print("{}".format(result_get))
        else:
          exit(-2)
    elif args.uttest:
      self.uttest(args.uttest)
    pass

