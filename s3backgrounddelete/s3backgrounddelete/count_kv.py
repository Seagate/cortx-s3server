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

""" To Count KV for a index_id you need to execute this file as:

    python36 count_kv.py index_id key threshold_value(optional) it's like 
    python36 count_kv.py sys.argv[1] sys.argv[2](optional)"""

import sys
from s3backgrounddelete.cortx_list_index_response import CORTXS3ListIndexResponse
from s3backgrounddelete.cortx_s3_error_respose import CORTXS3ErrorResponse
from s3backgrounddelete.cortx_s3_config import CORTXS3Config
from s3backgrounddelete.cortx_s3_index_api import CORTXS3IndexApi
from s3backgrounddelete.IEMutil import IEMutil

# Fetch threshold value if passed as command-line argument by user ,otherwise default value is None
def getthreshold():
    threshold = None
    if len(sys.argv) > 2:
        threshold = sys.argv[2]
    return threshold

# Count the kv entries and return the count
def count(dict):
    counter = 0
    for k,v in dict.items():
        if type(v) is list:
            for ele in v:
                for key in ele.keys():
                    if key == 'Key':
                        counter = counter + 1
    return counter

# Compare threshold value and total count of kv entries, raise IEM alert if count of kv entries exceeds threshold value and exit immediately
def checkthreshold(thr,kvcount):
    if(thr):
        if kvcount > int(thr):
            IEMutil("ERROR", IEMutil.THRESHOLD_CROSS, IEMutil.THRESHOLD_CROSS_STR)
            print("The kv entries are more than threshold value")
            sys.exit()

# Fetch the list of kv entries and get the total count of kv entries
def countkv():
    CONFIG = CORTXS3Config()
    index_api = CORTXS3IndexApi(CONFIG)
    response, data = index_api.list(sys.argv[1])
    if(response):
        if isinstance(data, CORTXS3ListIndexResponse):
            list_index_response = data.get_index_content()
            thld = getthreshold()
            total = count(list_index_response)
            checkthreshold(thld,total)
            print(f'Count iter0 : {total}')
            is_truncated = list_index_response["IsTruncated"]
            iter = 0
            while(is_truncated == "true"):
                response, data = index_api.list(sys.argv[1], max_keys=1000, next_marker=list_index_response["NextMarker"])
                if isinstance(data, CORTXS3ListIndexResponse):
                    list_index_response = data.get_index_content()
                    total = total + count(list_index_response)
                    checkthreshold(thld,total)
                    is_truncated = list_index_response["IsTruncated"]
                elif isinstance(data, CORTXS3ErrorResponse):
                    print(data.get_error_message())
                    break
                iter = iter + 1
                print(f'Count iter{iter} : {count(list_index_response)}')
            print(f'Total Count of kv entries : {total}')
    else:
        print("Error")
        print(data.get_error_message())

if __name__ == "__main__":
    countkv()
