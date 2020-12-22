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

conf_store = ConfStore()

def load_config(index, backend_url):
  """Instantiate and Load Config into constore"""
  conf_store.load(index, backend_url)
  return conf_store


class S3CortxConfStore:

  def __init__(self):
    print("constructor()")

  def load(self, index, path):
    print("in subroutine load()")
    result_data = load_config(index, path)

  def test_load(self, index, path):
    """This will be removed to ut directory later"""
    print("in subroutine test_load()")
    with open(urlparse(path).path) as f:
      jdata = json.load(f)
    load_config(index, path)
    result_data = conf_store.get(index, next(iter(jdata)), default_val=None)
    if result_data != None:
      print("test_load passed!")
    else:
      print("test_load failed!")

  def run(self):
    parser = argparse.ArgumentParser(description='Cortx-Utils ConfStore')
    parser.add_argument("--load", help='load the backing storage in an index')
    parser.add_argument("--test_load", help='test load the backing storage in an index')
    parser.add_argument("--path", help='example: JSON:///etc/cortx/myconf.json')
    args = parser.parse_args()

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

    if args.load or args.test_load:
      path = args.path
      if args.load:
        index = args.load
        self.load(index, path)
        print("load conf: {} into index: {}".format(path, index))
      else:
        index = args.test_load
        self.test_load(index, path)
        print("test load conf: {} into index: {}".format(path, index))
    else: 
      pass
