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
from s3recovery.s3recoverybase import S3RecoveryBase
from s3recovery.config import Config

class S3RecoveryDryRun(S3RecoveryBase):
    def __init__(self):
        super(S3RecoveryDryRun, self).__init__()

    def dry_run(self, index_name, index_id, index_id_replica):
        """
        Performs dry run on the index to be restored

        :index_name:  Name of index being processed
        :index_id: Id of index being processed
        :index_id_replica: Id of replica index being processed

        """
        union_result = dict()
        super(S3RecoveryDryRun, self).initiate(index_name, index_id, index_id_replica)
        super(S3RecoveryDryRun, self).dry_run(index_name, union_result, log_output = True)

    def start(self):
        """
        Entry point for dry run algorithm

        """
        self.dry_run("Global bucket index", Config.global_bucket_index_id,
                Config.global_bucket_index_id_replica)
        self.dry_run("Bucket metadata index", Config.bucket_metadata_index_id,
                Config.bucket_metadata_index_id_replica)
