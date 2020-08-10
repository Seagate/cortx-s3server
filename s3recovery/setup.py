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
files = ["VERSION"]

# Load the version
s3recovery_version = "1.0.0"
current_script_path = os.path.abspath(os.path.dirname(__file__))
with open(os.path.join(current_script_path, 'VERSION')) as version_file:
    s3recovery_version = version_file.read().strip()

setup(
    # Application name:
    name="s3recovery",

    # Version number (initial):
    version=s3recovery_version,

    # Application author details:
    author="Seagate",

    # Packages
    packages=["s3recovery"],

    # Include additional files into the package
    include_package_data=True,

    # Details
    scripts =['s3recovery/s3recovery'],

    # license="LICENSE.txt",
    description="Recover S3 from metatdata corruption",

    package_data = { 's3recovery': files}

)
