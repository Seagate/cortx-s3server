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

""" To Get KV for a key you need to execute this file as:

    python36 get_kv.py index_id key it's like 
    python36 get_kv.py sys.argv[1] sys.argv[2] """

import sys
from s3backgrounddelete.cortx_s3_config import CORTXS3Config
from s3backgrounddelete.cortx_s3_kv_api import CORTXS3KVApi
CONFIG = CORTXS3Config()
kv_api = CORTXS3KVApi(CONFIG)
response, data = kv_api.get(sys.argv[1], sys.argv[2])
if(response):
   get_kv_response = data.get_value()
   print(get_kv_response)
else:
   print("Error")
   print(str(data.get_error_status()))
   print(str(data.get_error_message()))
