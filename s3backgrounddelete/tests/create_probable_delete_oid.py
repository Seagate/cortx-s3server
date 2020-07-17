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

"""This will generate sample data and test s3 background delete functionality."""
#!/usr/bin/python3
import sys
import os

sys.path.append(
    os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir)))

from cortx_s3_config import CORTXS3Config
from cortx_s3_kv_api import CORTXS3KVApi
from cortx_s3_index_api import CORTXS3IndexApi
from cortx_s3_object_api import CORTXS3ObjectApi


# Create sample data for s3 background delete.
if __name__ == "__main__":
    CONFIG = CORTXS3Config()
    CORTXS3IndexApi(CONFIG).put("probable_delete_index_id")
    CORTXS3KVApi(CONFIG).put(
        "probable_delete_index_id",
        "oid-1",
        "{ \"obj-name\" : \"bucket_1/obj_1\"}")
    CORTXS3KVApi(CONFIG).put(
        "probable_delete_index_id",
        "oid-2",
        "{ \"obj-name\" : \"bucket_1/obj_2\"}")
    CORTXS3IndexApi(CONFIG).list("probable_delete_index_id")
    CORTXS3KVApi(CONFIG).get("probable_delete_index_id", "oid-1")
    CORTXS3KVApi(CONFIG).get("probable_delete_index_id", "oid-3")

    # Sample object metadata
    '''{
        "ACL" : "",
        "Bucket-Name" : "seagatebucket",
        "Object-Name" : "SC16Builds_v1.tar.gz",
        "Object-URI" : "seagatebucket\\\\SC16Builds_v1.tar.gz",
        "System-Defined": {
            "Content-Length" : "69435580",
            "Content-MD5" : "30baeecadf7417f842f5b9088b73f8a1-5",
            "Date" : "2016-10-24T09:01:03.000Z",
            "Last-Modified" : "2016-10-24T09:01:03.000Z",
            "Owner-Account" : "s3_test",
            "Owner-Account-id" : "12345",
            "Owner-User" : "tester",
            "Owner-User-id" : "123",
            "x-amz-server-side-encryption" : "None",
            "x-amz-server-side-encryption-aws-kms-key-id" : "",
            "x-amz-server-side-encryption-customer-algorithm" : "",
            "x-amz-server-side-encryption-customer-key" : "",
            "x-amz-server-side-encryption-customer-key-MD5" : "",
            "x-amz-storage-class" : "STANDARD",
            "x-amz-version-id" : "",
            "x-amz-website-redirect-location" : "None"
        },
        "User-Defined": {
            "x-amz-meta-s3cmd": "true",
            "motr_oid_u_hi" : "CzhfWNjoNAA=",
            "motr_oid_u_lo" : "FzVUkUG8V+0=",
            }
    '''

    OBJECTID1_METADATA = "{\"Object-Name\":\"bucket_1/obj_1\", \
                           \"x-amz-meta-s3cmd\": \"true\", \
                           \"motr_oid_u_hi\" : \"oid-1\", \
                           \"motr_oid_u_lo\" : \"FzVUkUG8V+0=\"}"

    OBJECTID2_METADATA = "{\"Object-Name\":\"bucket_1/obj_2\", \
                           \"x-amz-meta-s3cmd\": \"true\", \
                           \"motr_oid_u_hi\" : \"oid-2\", \
                           \"motr_oid_u_lo\" : \"FzVUkUG8V+0=\"}"

    CORTXS3IndexApi(CONFIG).put("object_metadata_index_id")
    CORTXS3KVApi(CONFIG).put(
        "object_metadata_index_id",
        "oid-1",
        OBJECTID1_METADATA)
    CORTXS3KVApi(CONFIG).put(
        "object_metadata_index_id",
        "oid-2",
        OBJECTID2_METADATA)
    CORTXS3IndexApi(CONFIG).list("object_metadata_index_id")
    CORTXS3KVApi(CONFIG).get("object_metadata_index_id", "oid-1")
    CORTXS3KVApi(CONFIG).get("object_metadata_index_id", "oid-2")
    CORTXS3ObjectApi(CONFIG).delete("0x-7200000000000000x0","test")
