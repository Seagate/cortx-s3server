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

""" To List KV for a index_id you need to execute this file as:

    python36 list_kv.py index_id key it's like 
    python36 list_kv.py sys.argv[1] """

import sys
from s3backgrounddelete.cortx_s3_config import CORTXS3Config
from s3backgrounddelete.cortx_s3_index_api import CORTXS3IndexApi
CONFIG = CORTXS3Config()
index_api = CORTXS3IndexApi(CONFIG)
response, data = index_api.list(sys.argv[1])
if(response):
        list_index_response = data.get_index_content()
        print(str(list_index_response))
        is_truncated = list_index_response["IsTruncated"]
        while(is_truncated == "true"):
             response, data = index_api.list(sys.argv[1], list_index_response["NextMarker"])
             list_index_response = data.get_index_content()
             is_truncated = list_index_response["IsTruncated"]
             print(str(list_index_response))
else:
        print("Error")
        print(data.get_error_message())
