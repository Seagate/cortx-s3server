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

#!/usr/bin/env python3

import argparse
from s3recovery.config import Config
from s3recovery.s3recoverydryrun import S3RecoveryDryRun
from s3recovery.s3recovercorruption import S3RecoverCorruption

class S3Recovery:

    def run(self):
        parser = argparse.ArgumentParser(description='S3-Metadata recovery tool',add_help=False)
        parser.add_argument('-h', '--help', action='help', default=argparse.SUPPRESS,
                    help='Show this help message and exit')
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
