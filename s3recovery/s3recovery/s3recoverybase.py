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

import os
import sys
import json
import dateutil.parser
import logging
import datetime
from s3backgrounddelete.cortx_s3_config import CORTXS3Config
from s3backgrounddelete.cortx_s3_index_api import CORTXS3IndexApi
from s3backgrounddelete.cortx_s3_kv_api import CORTXS3KVApi
from s3recovery.config import Config
from json import JSONDecodeError

class S3RecoveryBase:
    def __init__(self):
        self.config = CORTXS3Config(s3recovery_flag = True)
        self.index_api = CORTXS3IndexApi(self.config)
        self.kv_api = CORTXS3KVApi(self.config)
        self.log_result = False
        self.logger = None

    def create_logger(self, logger_name):
        """
        Create logger, file handler and formatter

        """
        self.logger = logging.getLogger(logger_name)
        self.logger.setLevel(level=logging.DEBUG)

        logfile = str(Config.log_dir + "/" + "s3recovery-" +
                datetime.datetime.utcnow().strftime("%Y-%m-%d-%H-%M-%S.%f")
                + ".log")

        fhandler = logging.FileHandler(logfile)
        fhandler.setLevel(level=logging.DEBUG)
        formatter = logging.Formatter("%(asctime)s - %(name)s - %(levelname)s - %(message)s")
        fhandler.setFormatter(formatter)
        self.logger.addHandler(fhandler)

    def create_logger_directory(self):
        """
        Create log directory if not exists

        """
        self._logger_directory = os.path.join(Config.log_dir)
        if not os.path.isdir(self._logger_directory):
            try:
                os.mkdir(self._logger_directory)
            except BaseException:
                self.s3recovery_log("error","Unable to create log directory at " + str(Config.log_dir))

    def s3recovery_log(self, log_level, msg):
        """
        Logger for s3recovery tool

        """
        # Display log message on console
        print(msg)

        # Log levels https://docs.python.org/3/library/logging.html#levels

        if self.logger is not None:
            if log_level == "info":
                self.logger.info(msg)
            elif log_level == "debug":
                self.logger.debug(msg)
            elif log_level == "warning":
                self.logger.warning(msg)
            elif log_level == "error":
                self.logger.error(msg)
            elif log_level == "critical":
                self.logger.critical(msg)

    def check_response(self, status, request, response, index_id, key):
        """
        Validate the response received from s3server

        :status: Boolean response flag which determine success/failure of api.
        :response: Success/Error response data received from the server.

        """
        if (status):
            self.s3recovery_log("info", "Operation "+ request +" KV for key " + key + " on index "+ index_id + " success")
        elif (response.get_error_status() == 404):
            self.s3recovery_log("info", "Key " + key + " does not exist in index "+ index_id)
        else:
            self.s3recovery_log("error", "Operation "+ request +" KV for key " + key + " on index "+ index_id+ " failed")
            self.s3recovery_log("error", "Response code "+ response.get_error_status())
            self.s3recovery_log("error", "Error reason "+ response.get_error_reason())

    def put_kv(self, index_id, key, value):
        """
        Puts the given key-value to corresponding index-id

        :index_id: S3 index_id on which the PUT operation needs to be performed
        :key: Key to be inserted
        :value: Value to be inserted

        """
        status, response = self.kv_api.put(index_id, key, value)
        self.check_response(status, "put", response, index_id, key)

    def perform_validation(self, key, data_to_restore, item_replica, union_result):
        """
        Compare epoch value and merges the value part of index and replica.

        :key: Key for which epoch needs to be validated
        :data_to_restore:  Structurized form of index to restore in KV.
        :item_replica: Replica index content corresponding to "Key"
        :union_result: Structurized form of result in KV.

        """
        if data_to_restore is None and item_replica is None:
            return

        if (data_to_restore is None):
            try:
                bucket_metadata_replica = json.loads(item_replica)
                # Missing epoch will be considered as corruption
                primary_epoch = bucket_metadata_replica["create_timestamp"]
            except (KeyError, JSONDecodeError, TypeError):
                self.s3recovery_log("error", "Failed to parse JSON or timestamp missing")
                return
            union_result[key] = item_replica
            return

        if (item_replica is None):
            try:
                bucket_metadata = json.loads(data_to_restore)
                # Missing epoch will be considered as corruption
                secondary_epoch = bucket_metadata["create_timestamp"]
            except (KeyError, JSONDecodeError, TypeError):
                self.s3recovery_log("error", "Failed to parse JSON or timestamp missing")
                return
            union_result[key] = data_to_restore
            return

        p_corruption = False
        s_corruption = False

        try:
            bucket_metadata = json.loads(data_to_restore)
            # Missing epoch will be considered as corruption
            primary_epoch = bucket_metadata["create_timestamp"]
        except (KeyError, JSONDecodeError, TypeError):
            p_corruption = True

        try:
            bucket_metadata_replica = json.loads(item_replica)
            # Missing epoch will be considered as corruption
            secondary_epoch = bucket_metadata_replica["create_timestamp"]
        except (KeyError, JSONDecodeError, TypeError):
            s_corruption = True

        if p_corruption and (not s_corruption):
            union_result[key] = item_replica
        elif s_corruption and (not p_corruption):
            union_result[key] = data_to_restore
        elif (not p_corruption) and (not s_corruption):
            # Compare epoch here
            bucket_metadata_date = dateutil.parser.parse(primary_epoch)
            bucket_metadata_replica_date = dateutil.parser.parse(secondary_epoch)

            # Overwrite only if replica contains latest datetime
            if bucket_metadata_date < bucket_metadata_replica_date:
                union_result[key] = item_replica
            else:
                union_result[key] = data_to_restore
        else:
            # Both entries corrupted
            pass

    def append_results(self, result):
        """
        Appends the results in case index response is truncated.
        :result:  Contents of truncated index to be appended

        """
        if(result):
            self.list_response.extend(result)

    def list_index(self, index_id):
        """
        Lists the corresponding index id.

        :index_id:  Id of index to be listed

        """
        self.list_response = None
        response, data = self.index_api.list(index_id)

        if (not response):
            self.s3recovery_log("error", "Error while listing index {}".format(index_id))
            sys.exit(1)

        self.index_list_response = data.get_index_content()
        is_truncated = self.index_list_response["IsTruncated"]
        fetch_marker = self.index_list_response["NextMarker"]
        self.list_response = self.index_list_response['Keys']

        while(is_truncated == "true"):
             response, data = self.index_api.list(index_id, next_marker = fetch_marker )
             if (not response):
                self.s3recovery_log("error", "Error while listing index {}".format(index_id))
                sys.exit(1)
             self.index_list_response = data.get_index_content()

             is_truncated = self.index_list_response["IsTruncated"]
             fetch_marker = self.index_list_response["NextMarker"]
             self.append_results(self.index_list_response['Keys'])

        return self.list_response


    def parse_index_list_response(self, data):
        """
        Parse list index response from S3server

        :data: S3server response for list index

        """
        kv_data = {}
        if data is not None:
            for item in data:
                key = item['Key']
                kv_data[key] = item['Value']
        return kv_data

    def merge_keys(self, index_name, data, replica):
        """
        Merges key part of index and replica to form union list.

        :index_name:  Name of index being processed
        :data: Content of index in KV format
        :replica: Content of replica index in KV format

        :return: A structurized list representation of the keys.

        """
        data_list = list()
        replica_list = list()

        if data is not None:
            data_list = list(data.keys())
        if replica is not None:
            replica_list = list(replica.keys())

        if (self.log_result):
            self.s3recovery_log("info", '#' * 60)
            self.s3recovery_log("info", "Primary index content for {}".format(index_name))
            self.s3recovery_log("info", '#' * 60 + "\n")
            self.print_content(data)
            self.s3recovery_log("info", '#' * 60)
            self.s3recovery_log("info", "Replica index content for {}   ".format(index_name))
            self.s3recovery_log("info", '#' * 60 + "\n")
            self.print_content(replica)

        result_list = data_list
        result_list.extend(item for item in replica_list if item not in data_list)
        return result_list

    def initiate(self, index_name, index_id, index_id_replica, log_output = False):
        """
        Initiates the dry run process

        :index_name:  Name of index being processed
        :index_id: Id of index being processed
        :index_id_replica: Id of replica index being processed

        """
        self.log_result = log_output

        data = self.list_index(index_id)
        data_replica = self.list_index(index_id_replica)

        self.data_as_dict = self.parse_index_list_response(data)
        self.replica_as_dict = self.parse_index_list_response(data_replica)

        self.result = self.merge_keys(index_name, self.data_as_dict, self.replica_as_dict)

    def print_content(self, kv):
        """
        Prints KV content in readable format

        :kv:  Content to be printed

        """
        if bool(kv):
            for key, value in kv.items() :
                self.s3recovery_log("info", str(key+" "+value))
        else:
            self.s3recovery_log("info", "Empty\n")

    def dry_run(self, index_name, index_id, index_id_replica, union_result):
        """
        Gets latest value from index and its replica

        :return: A structurized dictonary of data to be restored

        """

        for key in self.result:
            try:
                metadata_value = self.data_as_dict[key]
            except KeyError:
                metadata_value = None

            try:
                replica_value = self.replica_as_dict[key]
            except KeyError:
                replica_value = None

            self.perform_validation(key, metadata_value, replica_value, union_result)

        if (self.log_result):
            self.s3recovery_log("info", '#' * 60)
            self.s3recovery_log("info", "Data recovered from both indexes for {}".format(index_name))
            self.s3recovery_log("info", '#' * 60 + "\n")
            self.print_content(union_result)

        return union_result


