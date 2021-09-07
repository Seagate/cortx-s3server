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

""" To Count KV for a index_id """

import sys
from s3backgrounddelete.cortx_list_index_response import CORTXS3ListIndexResponse
from s3backgrounddelete.cortx_s3_error_respose import CORTXS3ErrorResponse
from s3backgrounddelete.cortx_s3_config import CORTXS3Config
from s3backgrounddelete.cortx_s3_index_api import CORTXS3IndexApi
from s3backgrounddelete.cortx_s3_constants import CONNECTION_TYPE_PRODUCER

class CORTXS3Countkv:
       
    # Count the kv entries and return the count
    def itercount(self, kv_dict):
        counter = 0
        for value in kv_dict.values():
            if isinstance(value, list):
                for ele in value:
                    for key in ele.keys():
                        if key == 'Key':
                            counter = counter + 1
        return counter

    # Compare threshold value and total count of kv entries, if count of kv entries exceeds threshold value, exit immediately
    def checkthreshold(self, thld, kvcount):
        if(thld):
            if kvcount > int(thld):
                print("The kv entries are more than threshold value")
                sys.exit()

    # Fetch the list of kv entries and get the total count of kv entries
    def count(self, index_id="AAAAAAAAAHg=-AwAQAAAAAAA=", threshold=None):
        CONFIG = CORTXS3Config()
        index_api = CORTXS3IndexApi(CONFIG, connectionType=CONNECTION_TYPE_PRODUCER)
        response, data = index_api.list(index_id)
        if(response):
            if isinstance(data, CORTXS3ListIndexResponse):
                list_index_response = data.get_index_content()
                total = self.itercount(list_index_response)
                self.checkthreshold(threshold,total)
                print(f'Iteration count0 : {total}')
                is_truncated = list_index_response["IsTruncated"]
                itr = 0
                while(is_truncated == "true"):
                     response, data = index_api.list(index_id, max_keys=1000, next_marker=list_index_response["NextMarker"])
                     if isinstance(data, CORTXS3ListIndexResponse):
                         list_index_response = data.get_index_content()
                         total = total + self.itercount(list_index_response)
                         self.checkthreshold(threshold,total)
                         is_truncated = list_index_response["IsTruncated"]
                     elif isinstance(data, CORTXS3ErrorResponse):
                         print(data.get_error_message())
                         break
                     itr = itr + 1
                     print(f'Iteration count{itr} : {self.itercount(list_index_response)}')
                print(f'Total Count of kv entries : {total}')
        else:
            print("Error")
            print(data.get_error_message())

