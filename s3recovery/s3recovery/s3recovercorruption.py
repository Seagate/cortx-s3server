#!/usr/bin/python3.6

import sys
import json
import dateutil.parser
from s3backgrounddelete.eos_core_config import EOSCoreConfig
from s3backgrounddelete.eos_core_index_api import EOSCoreIndexApi
from s3backgrounddelete.eos_core_kv_api import EOSCoreKVApi
from s3recovery.config import Config

class S3RecoverCorruption:
    def __init__(self):
        self.config = EOSCoreConfig()
        self.index_api = EOSCoreIndexApi(self.config)
        self.kv_api = EOSCoreKVApi(self.config)

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

    def print_recovered_data(self, index_name, data_to_restore):
        print("Metadata recovered from " + index_name +"\n")
        print("Bucket-Name       Bucket-Metadata\n")
        for key, val in data_to_restore.items():
            print(str(key) +"     "+str(val["Value"]))

    def recover_index(self, index_id, data_to_restore):
        for key, val in data_to_restore.items():
            self.put_kv(index_id, str(key), str(val["Value"]))


    def recover_corruption(self, index_name, index_id, index_id_replica):

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

        print("Recovering metadata for " + index_name + "......")
        self.recover_index(index_id, data_to_restore)
        print("Success\n")


    def start(self):

        self.recover_corruption("global_bucket_index_id", Config.global_bucket_index_id, Config.global_bucket_index_id_replica)
        self.recover_corruption("bucket_metadata_index_id", Config.bucket_metadata_index_id, Config.bucket_metadata_index_id_replica)

