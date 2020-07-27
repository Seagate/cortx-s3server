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

#!/usr/bin/env python3

import argparse
from s3recovery.config import Config
from s3recovery.s3recoverydryrun import S3RecoveryDryRun
from s3recovery.s3recovercorruption import S3RecoverCorruption

class S3Recovery:

    def run(self):
        parser = argparse.ArgumentParser(description='S3-Metadata recovery tool')
        parser.add_argument("--dry_run", help="Dry run of S3-Metadata corruption recovery",
                    action="store_true")
        parser.add_argument("--recover", help="Recover S3-Metadata corruption (Silent)",
                    action="store_true")
        args = parser.parse_args()

        if args.dry_run:
            action = S3RecoveryDryRun()
            action.start()
        else:
            pass

        if args.recover:
            action = S3RecoverCorruption()
            action.start()
        else:
            pass
