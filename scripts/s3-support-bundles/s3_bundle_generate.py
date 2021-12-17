#!/usr/bin/env python3

# Copyright (c) 2021 Seagate Technology LLC and/or its Affiliates
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as published
# by the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU Affero General Public License for more details.
# You should have received a copy of the GNU Affero General Public License
# along with this program. If not, see <https://www.gnu.org/licenses/>.
# For any questions about this software or licensing,
# please email opensource@seagate.com or cortx-questions@seagate.com.

import sys
import os
import shutil
import tarfile
import argparse

class S3SupportBundle:
    """ Generate support bundle for S3."""

    bundle_id=NotImplemented
    target_path=None
    cluster_conf=None
    services=None

    def generate_bundle_common(self):
        """ Generate support bundle for the S3 common logs"""
        print("Collect Support logs for common started")
        print("Collect Support logs for common completed")

    def generate_bundle_s3(self):
        """ Generate support bundle for the S3 Server logs"""
        print("Collect Support logs for S3 server started")
        print("Collect Support logs for S3 Server completed")

    def generate_bundle_authserver(self):
        """ Generate support bundle for the Authserver logs"""
        print("Collect Support logs for AuthServer started")
        print("Collect Support logs for AuthServer completed")

    def generate_bundle_bgscheduler(self):
        """ Generate support bundle for the BG delete scheduler logs"""
        print("Collect Support logs for BG delete scheduler started")
        print("Collect Support logs for BG delete scheduler completed")

    def generate_bundle_bgworker(self):
        """ Generate support bundle for the BG delete worker logs"""
        print("Collect Support logs for BG delete worker started")
        print("Collect Support logs for BG delete worker completed")

    def generate_bundle_haproxy(self):
        """ Generate support bundle for the haproxy logs"""
        print("Collect Support logs for haproxy started")
        print("Collect Support logs for haproxy completed")

    def generate_bundle(self):
        """ Generate support bundle for the S3 logs"""
        print(f"bundle_id: {self.bundle_id}")
        print(f"target_path: {self.target_path}")
        print(f"cluster_conf: {self.cluster_conf}")
        print(f"services: {self.services}")

        # parse services param

def main():
    parser = argparse.ArgumentParser(description='''Support Bundle for S3 logs.''')
    parser.add_argument('-b', dest='bundle_id', help='Unique bundle id')
    parser.add_argument('-t', dest='path', help='Path to store the created bundle')
    parser.add_argument('-c', dest='cluster_conf', help="Cluster config file path")
    parser.add_argument('-s', dest='services', help='List of services for Support Bundle')
    args = parser.parse_args()
    S3SupportBundle(args.bundle_id, args.path, args.cluster_conf, args.services).generate_bundle()

if __name__ == '__main__':
    main()