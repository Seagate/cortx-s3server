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
