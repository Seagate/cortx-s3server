#!/usr/bin/env python3

import os
import sys
import argparse
from s3recovery.config import Config
from s3recovery.s3recoverydryrun import S3RecoveryDryRun
from s3recovery.s3recovercorruption import S3RecoverCorruption
from s3recovery.s3recoverinteractive import S3RecoverInteractive

class S3Recovery:

    def run(self):
        parser = argparse.ArgumentParser(description='S3-Metadata recovery tool')
        parser.add_argument("--dry_run", help="Dry run to recovery S3-Metadata corruption",
                    action="store_true")
        parser.add_argument("--recover_corruption", help="Recovery S3-Metadata corruption",
                    action="store_true")
        parser.add_argument("--recover_interactive", help="Recovery S3-Metadata corruption in interactive mode",
                    action="store_true")
        args = parser.parse_args()

        if args.dry_run:
            action = S3RecoveryDryRun()
            action.start()
        else:
            pass

        if args.recover_corruption:
            action = S3RecoverCorruption()
            action.start()
        else:
            pass

        if args.recover_interactive:
            action = S3RecoverInteractive()
            action.start()
        else:
            pass