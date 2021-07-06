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

import os
from setuptools import setup
import sys
files = ["config/*"]

# Load the version
s3backgrounddelete_version = "2.0.0"

for argument in sys.argv:
    if argument.startswith("--version"):
        s3backgrounddelete_version = argument.split("=")[1]
        sys.argv.remove(argument)

setup(
    # Application name:
    name="s3backgrounddelete",

    # Version number (initial):
    version=s3backgrounddelete_version,

    # Application author details:
    author="Seagate",

    # Packages
    packages=["s3backgrounddelete"],

    # Include additional files into the package
    include_package_data=True,

    # Details
    scripts =['s3backgrounddelete/s3backgroundproducer', 's3backgrounddelete/s3backgroundconsumer'],

    # license="LICENSE.txt",
    description="Background delete process to clean stale oids in s3",

    package_data = { 's3backgrounddelete': files},

    # Dependent packages (distributions)
    install_requires=[
    'httplib2'
  ]
)
