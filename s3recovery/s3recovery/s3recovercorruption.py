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
from s3recovery.s3recoverybase import S3RecoveryBase
from s3recovery.config import Config

METADATA_INDEX_DELIMITER = '/'

class S3RecoverCorruption(S3RecoveryBase):
    def __init__(self):
        super(S3RecoverCorruption, self).__init__()
        super(S3RecoverCorruption, self).create_logger_directory()
        super(S3RecoverCorruption, self).create_logger("S3RecoverCorruption")

    def cleanup_bucket_list_entries(self, index_id):
        """
        Performs cleanup of stale data entries

        """
        list_index_response = self.list_index(index_id)
        data_as_dict = self.parse_index_list_response(list_index_response)
        key_list = list(data_as_dict.keys())
        for key in key_list:
            if key not in self.common_keys:
                status, response = self.kv_api.delete(index_id, key)
                super(S3RecoverCorruption, self).check_response(status, "delete", response, index_id, key)

    def cleanup_bucket_metadata_entries(self, index_id):
        """
        Performs cleanup of stale data entries
        """

        list_index_response = self.list_index(index_id)
        data_as_dict = self.parse_index_list_response(list_index_response)
        key_list = list(data_as_dict.keys())
        for key in key_list:
            if (key.count(METADATA_INDEX_DELIMITER) == 1):
                key_part = key.split(METADATA_INDEX_DELIMITER)[1]
                if key_part not in self.common_keys:
                    status, response = self.kv_api.delete(index_id, key)
                    super(S3RecoverCorruption, self).check_response(status, "delete", response, index_id, key)
            else:
                # Key corruption case. Perform cleanup
                status, response = self.kv_api.delete(index_id, key)
                super(S3RecoverCorruption, self).check_response(status, "delete", response, index_id, key)

    def restore_data(self, list_index_id, list_index_id_replica, metadata_index_id,
            metadata_index_id_replica):
        """
        Performs recovery of data to be restored

        """
        if ((not self.list_result) or (not self.metadata_result)):
            self.s3recovery_log("info", "No any data to recover\n")
            return

        self.s3recovery_log("info", '#' * 60)
        self.s3recovery_log("info", "Recovering global list index table")
        self.s3recovery_log("info", '#' * 60 + "\n")
        for key, value in self.list_result.items():
            if key in self.common_keys:
                self.s3recovery_log("info", "\nRecovering {} {}".format(key,value))
                super(S3RecoverCorruption, self).put_kv(list_index_id, key, value)
                super(S3RecoverCorruption, self).put_kv(list_index_id_replica, key, value)

        self.cleanup_bucket_list_entries(list_index_id)
        self.cleanup_bucket_list_entries(list_index_id_replica)

        """ Sample entry in bucket metadata table
        i.e self.metadata_result contents

        {'12345/test': '{"ACL":"","Bucket-Name":"test","Policy":"","System-Defined"
        :{"Date":"2020-07-07T11:30:53.000Z","LocationConstraint":"us-west-2",
        "Owner-Account":"s3_test","Owner-Account-id":"12345","Owner-User":"tester",
        "Owner-User-id":"123"},"create_timestamp":"2020-07-07T11:30:53.000Z",
        "motr_multipart_index_oid":"dQmoBAAAAHg=-AgAAAAAAtMU=",
        "motr_object_list_index_oid":"dQmoBAAAAHg=-AQAAAAAAtMU=",
        "motr_objects_version_list_index_oid":"dQmoBAAAAHg=-AwAAAAAAtMU="}
        }
        """

        self.s3recovery_log("info", "\n"+ '#' * 60)
        self.s3recovery_log("info", "Recovering bucket metadata table")
        self.s3recovery_log("info", '#' * 60 + "\n")
        for key, value in self.metadata_result.items():
            if (key.count(METADATA_INDEX_DELIMITER) == 1):
                if key.split(METADATA_INDEX_DELIMITER)[1] in self.common_keys:
                    self.s3recovery_log("info", "\nRecovering {} {}".format(key,value))
                    super(S3RecoverCorruption, self).put_kv(metadata_index_id, key, value)
                    super(S3RecoverCorruption, self).put_kv(metadata_index_id_replica, key, value)

        self.cleanup_bucket_metadata_entries(metadata_index_id)
        self.cleanup_bucket_metadata_entries(metadata_index_id_replica)

        self.s3recovery_log("info", "\nS3recovery passed successfully...!!!")


    def check_consistency(self, list_index_id, list_index_id_replica, metadata_index_id, metadata_index_id_replica):
        """
        Performs consistency check of indexes to be restored

        """
        self.common_keys = []
        if ((not self.list_result) and (not self.metadata_result)):
            self.list_result = {}
            self.metadata_result = {}
            return

        if (not self.list_result):
            self.s3recovery_log("info", "GBLI empty - Cleaning up GBMI\n")
            self.cleanup_bucket_metadata_entries(metadata_index_id)
            self.cleanup_bucket_metadata_entries(metadata_index_id_replica)
            self.list_result = {}
            self.metadata_result = {}
            return

        if (not self.metadata_result):
            self.s3recovery_log("info", "GBMI empty - Cleaning up GBLI\n")
            self.cleanup_bucket_list_entries(list_index_id)
            self.cleanup_bucket_list_entries(list_index_id_replica)
            self.list_result = {}
            self.metadata_result = {}
            return

        global_key_list = list(self.list_result.keys())
        global_metadata_list = list(self.metadata_result.keys())

        for item in global_metadata_list:
            if (item.count(METADATA_INDEX_DELIMITER) == 1):
                entry = item.split(METADATA_INDEX_DELIMITER)[1]
                if (entry in global_key_list):
                    self.common_keys.append(entry)


    def recover_corruption(self, list_index_name, list_index_id, list_index_id_replica,
            metadata_index_name, metadata_index_id, metadata_index_id_replica):
        """
        Performs recovery of index to be restored

        :list_index_name:  Name of list index being processed
        :list_index_id: Id of list index being processed
        :index_id_rlist_index_id_replicaeplica: Id of list replica index being processed
        :metadata_index_name:  Name of metadata index being processed
        :metadata_index_id: Id of metadata index being processed
        :metadata_index_id_replica: Id of metadata replica index being processed

        """
        union_result = dict()
        super(S3RecoverCorruption, self).initiate(list_index_name, list_index_id,
                list_index_id_replica, log_output = True)
        self.list_result = super(S3RecoverCorruption, self).dry_run(list_index_name, list_index_id,
                list_index_id_replica, union_result)

        metadata_result = dict()
        super(S3RecoverCorruption, self).initiate(metadata_index_name, metadata_index_id,
                metadata_index_id_replica, log_output = True)
        self.metadata_result = super(S3RecoverCorruption, self).dry_run(metadata_index_name, metadata_index_id,
                metadata_index_id_replica, metadata_result)

        self.check_consistency(list_index_id, list_index_id_replica, metadata_index_id, metadata_index_id_replica)
        self.restore_data(list_index_id, list_index_id_replica, metadata_index_id,
                metadata_index_id_replica)

    def start(self):
        """
        Entry point for recover algorithm

        """
        self.recover_corruption("Global bucket index", Config.global_bucket_index_id,
            Config.global_bucket_index_id_replica, "Bucket metadata index",
            Config.bucket_metadata_index_id, Config.bucket_metadata_index_id_replica)
