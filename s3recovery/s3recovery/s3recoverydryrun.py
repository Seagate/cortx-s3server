#!/usr/bin/python3.6

import sys
import json
import dateutil.parser
from s3recovery.s3recoverybase import S3RecoveryBase
from s3recovery.config import Config

class S3RecoveryDryRun(S3RecoveryBase):
    def __init__(self):
        super(S3RecoveryDryRun, self).__init__()
        super(S3RecoveryDryRun, self).create_logger_directory()
        super(S3RecoveryDryRun, self).create_logger("S3RecoveryDryRun")

    def dry_run(self, index_name, index_id, index_id_replica):
        """
        Performs dry run on the index to be restored

        :index_name:  Name of index being processed
        :index_id: Id of index being processed
        :index_id_replica: Id of replica index being processed

        """
        union_result = dict()
        super(S3RecoveryDryRun, self).initiate(index_name, index_id, index_id_replica, log_output = True)
        super(S3RecoveryDryRun, self).dry_run(index_name, index_id, index_id_replica, union_result, recover_flag = False)

    def start(self):
        """
        Entry point for dry run algorithm

        """
        self.dry_run("Global bucket index", Config.global_bucket_index_id,
                Config.global_bucket_index_id_replica)
        self.dry_run("Bucket metadata index", Config.bucket_metadata_index_id,
                Config.bucket_metadata_index_id_replica)
