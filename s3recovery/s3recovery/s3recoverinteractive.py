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

#!/usr/bin/python3.6

import sys
import json
import dateutil.parser
from s3backgrounddelete.cortx_s3_config import CORTXS3Config
from s3backgrounddelete.cortx_s3_index_api import CORTXS3IndexApi
from s3backgrounddelete.cortx_s3_kv_api import CORTXS3KVApi
from s3recovery.config import Config

class S3RecoverInteractive:
    def __init__(self):
        self.config = CORTXS3Config()
        self.index_api = CORTXS3IndexApi(self.config)
        self.kv_api = CORTXS3KVApi(self.config)

    def put_kv(self, index_id, key, value):
        # Check for S3 putkv support
        response, data = self.kv_api.put(index_id, key, value)

    def perform_validation(self, data_to_restore, key, item_replica):
        # Compare epoch here
        bucket_metadata = json.loads(data_to_restore[key]["Value"])
        bucket_metadata_replica = json.loads(item_replica["Value"])

        bucket_metadata_date = dateutil.parser.parse(bucket_metadata["create_timestamp"])
        bucket_metadata_replica_date = dateutil.parser.parse(bucket_metadata_replica["create_timestamp"])

        # Overwrite only if replica contains latest datetime
        if bucket_metadata_date < bucket_metadata_replica_date:
            data_to_restore[key] = item_replica

    def list_index(self, index_id):
        response, data = self.index_api.list(index_id)

        if not response:
            print("Error while listing index {}".format(index_id))
            sys.exit(1)

        index_list_response = data.get_index_content()
        return index_list_response['Keys']

    def user_prompt(self, query, default=None):

        supported_values = {"yes": True, "y": True, "no": False, "n": False}
        options = " [yes/no] "

        while True:
            sys.stdout.write(query + options)
            choice = input().lower()
            if default is not None and choice == '':
                return supported_values[default]
            elif choice in supported_values:
                return supported_values[choice]
            else:
                sys.stdout.write("Please respond with 'yes' or 'no' "
                                "(or 'y' or 'n').\n")

    def recover_index(self, index_name, index_id, data_to_restore):
        print("Recovering metadata for " + index_name +"\n")
        for key, val in data_to_restore.items():
            if (self.user_prompt("Do you want to restore metadata for bucket entry \n"+ str(key) + "    " + str(val))):
                self.put_kv(index_id, str(key), str(val["Value"]))
            else:
                print("Bucket metadata entry skipped")


    def interactive_recover(self, index_name, index_id, index_id_replica):

        data = self.list_index(index_id)
        data_replica = self.list_index(index_id_replica)

        # Merge unique contents index and it replica
        data_to_restore = {}
        for item in data:
            key = item['Key']
            data_to_restore[key] = item

        # Insert remaining entries from replica 
        for item_replica in data_replica:
            key = item_replica['Key']
            if key not in data_to_restore:
                data_to_restore[key] = item_replica
            else:
                self.perform_validation(data_to_restore, key, item_replica)

        self.recover_index(index_name, index_id, data_to_restore)

    def start(self):

        self.interactive_recover("global_bucket_index_id", Config.global_bucket_index_id, Config.global_bucket_index_id_replica)
        self.interactive_recover("bucket_metadata_index_id", Config.bucket_metadata_index_id, Config.bucket_metadata_index_id_replica)

