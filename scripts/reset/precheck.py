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

# Sanity check to validate s3 global indexes are empty
# Prerequisites : S3backgroundelete should be installed
#                 S3server should be running

import sys
from s3backgrounddelete.cortx_s3_config import CORTXS3Config
from s3backgrounddelete.cortx_s3_index_api import CORTXS3IndexApi

config = CORTXS3Config()

def assert_index(index_name, index_id, expected_kv_count=0, error_message=None):

    print("Checking {}".format(index_name))
    index_api = CORTXS3IndexApi(config)

    # Instance index has startup entries, other indexes should be empty.
    max_keys = 1
    if expected_kv_count > 0:
        max_keys = expected_kv_count

    response, data = index_api.list(index_id, max_keys)

    if response:
        list_index_response = data.get_index_content()
        keys = list_index_response['Keys']

        if expected_kv_count == 0:
            if keys is not None:
                print("Index {} is not empty".format(index_name))
                if error_message is not None:
                    print(error_message)
                sys.exit(1)
        else:
            if len(keys) != expected_kv_count:
                print("Index {} does not have expected entries".format(index_name))
                if error_message is not None:
                    print(error_message)
                sys.exit(1)
        print("Success")

    # Index listing failed. S3server might be down.
    else:
        print("Error while listing index {}".format(index_name))
        sys.exit(1)

if __name__ == "__main__":

    # Assert all S3server indexes are clean during starup
    assert_index("global_bucket_index", config.get_global_bucket_index_id(),
                 expected_kv_count=0)
    assert_index("bucket_metadata_index", config.get_bucket_metadata_index_id(),
                 expected_kv_count=0)
    assert_index("global_probable_delete_index",
                 config.get_probable_delete_index_id(), expected_kv_count=0)

    err_message = "Possible invalid instance metadata for running S3 services." \
                   "Try restarting S3 services after kvs cleanup."

    assert_index("global_instance_index", config.get_global_instance_index_id(),
                 expected_kv_count=config.get_s3_instance_count(),
                 error_message=err_message)
