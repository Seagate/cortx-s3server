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
from cortx.utils.conf_store import Conf
from urllib.parse import urlparse
import os.path
import json
import sys
import inspect
from cortx.utils.kv_store import kv_store_collection

class S3CortxConfStore:
  default_index = "default_cortx_s3_confstore_index"

  def __init__(self):
    """Instantiate constore"""

  def load_config(self, index: str, backend_url: str):
    """Load Config into constore"""
    if not backend_url.strip():
      print("Cannot load py-utils:confstore as backend_url[{}] is empty.".format(backend_url))
      exit(-1)
    else:
      Conf.load(index, backend_url)

  def get_config(self, index: str, key: str):
    """Get the key's config from constore"""
    return Conf.get(index, key)

  def set_config(self, index: str, key: str, setval: str, save: bool = False):
    """Set the key's value in constore"""
    Conf.set(index, key, setval)
    if save == True:
      """Update the index backend"""
      Conf.save(index)

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

    self.set_config(index, 'cluster>cluster_id', '1234', False)
    result_data = self.get_config(index, 'cluster>cluster_id')
    if result_data != '1234':
      print("set_config() failed!")
      os.remove("/tmp/cortx_s3_confstoreuttest.json")
      exit(-1)

    os.remove("/tmp/cortx_s3_confstoreuttest.json")
    print("cortx_s3_confstore unit test passed!")

  def run(self):
    parser = argparse.ArgumentParser(description='Cortx-Utils ConfStore')
    parser.add_argument("--load", help='Load the backing storage in this index, pass --path to config')
    parser.add_argument("--get", help='Obtain config for this given index using a key, pass --key')
    parser.add_argument("--set", help='Sets config value for this given index using a key, pass --key and --setval')
    parser.add_argument("--persistent", help='Optional, pass <Yes> if needed, to be used with --set, this updates the backend along with --set')
    parser.add_argument("--key", help='Provide a key to be used in --get and --set')
    parser.add_argument("--getkey", help='Fetch value of the given key from default index, pass --path to config')
    parser.add_argument("--setkey", help='Sets config value for the given key from default index, pass --path to config and --setval')
    parser.add_argument("--setval", help='String value to be set using --set or --setkey')
    parser.add_argument("--path", help='Path to config file, supported formats are: json, toml, yaml, ini, pillar (salt). Usage: <format>://<file_path>, e.g. JSON:///etc/cortx/myconf.json')
    parser.add_argument("--uttest", help='An unit test for the load, get, get_data and set APIs')

    if len(sys.argv) < 2:
      print("Incorrect args, use -h to review usage")
      exit(-1)

    args = parser.parse_args()

    if args.path:
      if not args.path.strip():
        print("--path option value cannot be empty string.")
        exit(-1)
      if args.load == None and args.getkey == None and args.setkey == None:
        print("--path option is required only for --load, --getkey and --setkey options")
        exit(-1)
      if os.path.isfile(urlparse(args.path).path) != True:
        print("config file: {} does not exist".format(args.path))
        exit(-1)
      else:
        store_type = urlparse(args.path).scheme
        is_valid_type = False
        valid_types = ''
        storage = inspect.getmembers(kv_store_collection, inspect.isclass)
        for name, cls in storage:
          if hasattr(cls, 'name') and name != "KvStore":
            valid_types += cls.name + ' '
            if store_type == cls.name:
              is_valid_type = True
              break

        if is_valid_type == False:
          print("Invalid storage type {} in config file: {}, accepted types are {}".format(store_type, args.path, valid_types))
          exit(-1)

        if store_type == 'json':
          try:
            with open(urlparse(args.path).path) as f:
              json.load(f)
          except ValueError as e:
            print("config file: {} must use valid JSON format: {}".format(urlparse(args.path).path, e))
            exit(-1)
        """TODO: Implement rest of the type's content validators here"""

    if args.key and args.get == None and args.set == None:
      print("--key is needed by only --get and --set")
      exit(-1)

    if args.setval and args.set == None and args.setkey == None:
      print("--setval is needed by only --set and --setkey")
      exit(-1)

    if args.persistent == "Yes" and args.set == None:
      print("--persistent option to be used only with --set")
      exit(-1)

    if args.get:
      if args.key == None:
        print("Please provide the key to be get from confstore using --key")
        exit(-1)
      result_get = self.get_config(args.get, args.key)
      if result_get:
        print("{}".format(result_get))

    if args.set:
      if args.key == None:
        print("Please provide the key to be set in confstore using --key")
        exit(-1)
      if args.setval == None:
        print("Please provide the value to be set in string format using --setval")
        exit(-1)
      if args.persistent and args.persistent != "Yes":
        print("Please provide Yes as the valid value for --persistent")
        exit(-1)
      self.set_config(args.set, args.key, args.setval, args.persistent == "Yes")

    if args.load or args.getkey or args.setkey:
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

      elif args.setkey:
        if args.setval == None:
          print("Please provide the value to be set in string format using --setval")
          exit(-1)
        self.load_config(self.default_index, urlpath)
        self.set_config(self.default_index, args.setkey, args.setval, True)

    elif args.uttest:
      self.uttest(args.uttest)
    pass

