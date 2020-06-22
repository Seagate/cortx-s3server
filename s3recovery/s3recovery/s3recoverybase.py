'''
 COPYRIGHT 2020 SEAGATE LLC

 THIS DRAWING/DOCUMENT, ITS SPECIFICATIONS, AND THE DATA CONTAINED
 HEREIN, ARE THE EXCLUSIVE PROPERTY OF SEAGATE TECHNOLOGY
 LIMITED, ISSUED IN STRICT CONFIDENCE AND SHALL NOT, WITHOUT
 THE PRIOR WRITTEN PERMISSION OF SEAGATE TECHNOLOGY LIMITED,
 BE REPRODUCED, COPIED, OR DISCLOSED TO A THIRD PARTY, OR
 USED FOR ANY PURPOSE WHATSOEVER, OR STORED IN A RETRIEVAL SYSTEM
 EXCEPT AS ALLOWED BY THE TERMS OF SEAGATE LICENSES AND AGREEMENTS.

 YOU SHOULD HAVE RECEIVED A COPY OF SEAGATE'S LICENSE ALONG WITH
 THIS RELEASE. IF NOT PLEASE CONTACT A SEAGATE REPRESENTATIVE
 http://www.seagate.com/contact

 Original author: Siddhivinayak Shanbhag  <siddhivinayak.shanbhag@seagate.com>
 Original creation date: 24-Jun-2020
'''

#!/usr/bin/python3.6

import sys
import json
import dateutil.parser
from s3backgrounddelete.eos_core_config import EOSCoreConfig
from s3backgrounddelete.eos_core_index_api import EOSCoreIndexApi
from s3backgrounddelete.eos_core_kv_api import EOSCoreKVApi
from s3recovery.config import Config

class S3RecoveryBase:
    def __init__(self):
        self.config = EOSCoreConfig()
        self.index_api = EOSCoreIndexApi(self.config)
        self.kv_api = EOSCoreKVApi(self.config)

    def put_kv(self, index_id, key, value):
        """
        Puts the given key-value to corresponding index-id

        :index_id: S3 index_id on which the PUT operation needs to be performed
        :key: Key to be inserted
        :value: Value to be inserted

        """
        # Check for S3 putkv support
        response, data = self.kv_api.put(index_id, key, value)

    def perform_validation(self, key, data_to_restore, item_replica, union_result):
        """
        Compare epoch value and merges the value part of index and replica.

        :data_to_restore:  Structurized form of index to restore in KV.
        :key: Key for which epoch needs to be validated
        :item_replica: Replica index content corresponding to "Key"
        :union_result: Structurized form of result in KV.

        """
        if (data_to_restore is None) or (item_replica is None):
            print("Invalid values while parsing. Skip validation")
            return

        p_corruption = False
        s_corruption = False

        try:
            bucket_metadata = json.loads(data_to_restore)
        except JSONDecodeError:
            p_corruption = True

        try:
            bucket_metadata_replica = json.loads(item_replica)
        except JSONDecodeError:
            s_corruption = True

        if p_corruption and (not s_corruption):
            union_result[key] = data_to_restore
        elif s_corruption and (not p_corruption):
            union_result[key] = item_replica
        else:
            # Compare epoch here
            bucket_metadata_date = dateutil.parser.parse(bucket_metadata["create_timestamp"])
            bucket_metadata_replica_date = dateutil.parser.parse(bucket_metadata_replica["create_timestamp"])

            # Overwrite only if replica contains latest datetime
            if bucket_metadata_date < bucket_metadata_replica_date:
                union_result[key] = item_replica
            else:
                union_result[key] = data_to_restore

    def list_index(self, index_id):
        """
        Lists the corresponding index id.

        :index_id:  Id of index to be listed

        """
        response, data = self.index_api.list(index_id)

        if not response:
            print("Error while listing index {}".format(index_id))
            sys.exit(1)

        index_list_response = data.get_index_content()
        return index_list_response['Keys']

    def parse_index_list_response(self, data):
        """
        Parse list index response from S3server

        :data: S3server response for list index

        """
        kv_data = {}
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
        data_list = list(data.keys())
        replica_list = list(replica.keys())

        print("\nPrimary index content for {} \n".format(index_name))
        self.print_content(data)
        print("\nReplica index content for {} \n".format(index_name))
        self.print_content(replica)

        result_list = data_list
        result_list.extend(item for item in replica_list if item not in data_list)
        return result_list

    def initiate(self, index_name, index_id, index_id_replica):
        """
        Initiates the dry run process

        :index_name:  Name of index being processed
        :index_id: Id of index being processed
        :index_id_replica: Id of replica index being processed

        """
        data = self.list_index(index_id)
        data_replica = self.list_index(index_id_replica)

        self.data_as_dict = self.parse_index_list_response(data)
        self.replica_as_dict = self.parse_index_list_response(data_replica)

        self.result = self.merge_keys(index_name, self.data_as_dict, self.replica_as_dict)

    def print_content(self, kv):
        """
        Prints KV content in readable format

        :kv:  Conent to be printed

        """
        for key, value in kv.items() :
            print (key, value)

    def dry_run(self, index_name, union_result):
        """
        Gets latest value from index and its replica

        :return: A structurized dictonary of data to be restored

        """
        for key in self.result:
            self.perform_validation(key, self.data_as_dict[key], self.replica_as_dict[key], union_result)

        print("\nData recovered from both indexes for {} \n".format(index_name))
        self.print_content(union_result)


