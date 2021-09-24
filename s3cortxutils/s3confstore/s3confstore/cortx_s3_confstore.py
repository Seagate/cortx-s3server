#!/usr/bin/python3.6
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

  def __init__(self, config: str = None, index: str = "default_s3confstore_index"):
    """Instantiate confstore."""
    self.config_file = config
    self.default_index = index

    if config is not None:
      self.validate_configfile(config)
      self.load_config(self.default_index, self.config_file)

  @staticmethod
  def load_config(index: str = "default_s3confstore_index", config: str = ""):
    """Load Config into confstore."""
    # TODO recurse flag will be deprecated in future.
    Conf.load(index, config, recurse = True)

  def get_config(self, key: str):
    """Get the key's config from confstore."""
    return Conf.get(self.default_index, key)

  def set_config(self, key: str, value: str, save: bool = False):
    """Set the key's value in confstore."""
    Conf.set(self.default_index, key, value)
    if save == True:
      # Update the index backend.
      Conf.save(self.default_index)

  def get_all_keys(self, recurse: bool = True):
    """Get all the key value pairs from confstore."""
    # TODO recurse flag will be deprecated in future.
    # Do changes in all places wherever its applicable
    # refer validate_config_files() and phase_keys_validate() in setupcmd.py
    return Conf.get_keys(self.default_index, recurse = recurse)

  def delete_key(self, key: str, save: bool = False):
    """Deletes the specified key."""
    Conf.delete(self.default_index, key)
    if save == True:
      # Update the index backend.
      Conf.save(self.default_index)

  def merge_config(self, source_index:str, keys_to_include:list = None):
    """
    In-place replaces of keys specified in keys_to_include from source to destination.
    In case keys_to_include is empty all keys are replace in-place.
    """
    # TODO recurse flag will be deprecated in future.
    # Do changes in all places wherever its applicable
    # refer upgrade_config() in merge.py
    Conf.copy(source_index, self.default_index, keys_to_include, recurse = True)

  def save_config(self):
    """Saves to config file."""
    Conf.save(self.default_index)

  def get_machine_id(self):
    """Get machine id from the constore"""
    return Conf.machine_id

  @staticmethod
  def validate_configfile(configfile: str):
    """Validate the 'configfile' url, if its a valid file and of supported format."""

    if not configfile.strip():
      print("Invalid configfile path: {}".format(configfile))
      sys.exit(1)

    if os.path.isfile(urlparse(configfile).path) != True:
      print("config file: {} does not exist".format(configfile))
      sys.exit(1)
    else:
      store_type = urlparse(configfile).scheme
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
        print("Invalid storage type {} in config file: {}, accepted types are {}".format(store_type, configfile, valid_types))
        sys.exit(1)

      if store_type == 'json':
        try:
          with open(urlparse(configfile).path) as f:
            json.load(f)
        except ValueError as e:
          print("config file: {} must use valid JSON format: {}".format(urlparse(configfile).path, e))
          sys.exit(1)
      """TODO: Implement rest of the type's content validators here"""

  def run(self):
    parser = argparse.ArgumentParser(description='cortx-py-utils::ConfStore wrapper')

    parser.add_argument("config",
                        help='config file url, check cortx-py-utils::confstore for supported formats.',
                        type=str)

    subparsers = parser.add_subparsers(dest='command', title='comamnds')

    getkey = subparsers.add_parser('getkey', help='get value of given key from confstore')
    getkey.add_argument('--key', help='Fetch value of the given key', type=str, required=True)

    setkey = subparsers.add_parser('setkey', help='set given value to given key in confstore')
    setkey.add_argument('--key', help='set a new value for the key', type=str, required=True)
    setkey.add_argument('--value', help='set this value for the given key', type=str, required=True)

    subparsers.add_parser('getallkeys', help='Get the list of all the Key Values in the file')

    args = parser.parse_args()

    s3conf_store = S3CortxConfStore(args.config)

    if args.command == 'getkey':
      keyvalue = s3conf_store.get_config(args.key)
      if keyvalue:
        print("{}".format(keyvalue))
      else:
        sys.exit("Failed to get key:{}'s value".format(args.key))

    elif args.command == 'setkey':
      s3conf_store.set_config(args.key, args.value, True)

    elif args.command == 'getallkeys':
      keyvalue = s3conf_store.get_all_keys()
      if keyvalue:
        print("{}".format(keyvalue))
      else:
        sys.exit("Failed to get key:{}'s value".format(args.key))

    else:
      sys.exit("Invalid command option passed, see help.")
