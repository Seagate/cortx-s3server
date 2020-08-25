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

""" To Put KV for a key you need to execute this file as:

    python36 put_kv.py index_id key value it's like 
    python36 put_kv.py sys.argv[1] sys.argv[2] sys.argv[3]

    value should be a proper json string, for eg:
    \"ACL\":\"asd\". Invalid Json will be rejected by server. """

import sys
from s3backgrounddelete.cortx_s3_config import CORTXS3Config
from s3backgrounddelete.cortx_s3_kv_api import CORTXS3KVApi
CONFIG = CORTXS3Config()
kv_api = CORTXS3KVApi(CONFIG)
response, data = kv_api.put(sys.argv[1], sys.argv[2], sys.argv[3])
if(response):
    print("success")
else:
    print("Error")
    print(data.get_error_message())
