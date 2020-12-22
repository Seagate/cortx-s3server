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

class S3CortxConfStore:

  def __init__(self):
    print("constructor()")

  def load(self):
    print("in subroutine load()")

  def run(self):
    parser = argparse.ArgumentParser(description='Cortx-Utils ConfStore')
    parser.add_argument("--load", help='load the backing storage in an index')
    parser.add_argument("--path", help='example: JSON:///etc/cortx/myconf.json')
    args = parser.parse_args()

    if args.load and args.path:
      index = args.load
      path = args.path
      print("load conf: {} into index: {}".format(path, index))
      self.load()
    else: 
      pass
