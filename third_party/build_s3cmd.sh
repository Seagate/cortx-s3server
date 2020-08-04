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

#!/bin/sh -xe
# Script to build s3cmd.
# git repo: https://github.com/s3tools/s3cmd.git
# branch: previous develop commit: 4c2489361d353db1a1815172a6143c8f5a2d1c40 (1.6.1)
# Current develop commit: 4801552f441cf12dec53099a6abc2b8aa36ccca4 (2.0.2)

cd s3cmd

# Apply s3cmd patch
#patch -f -p1 < ../../patches/s3cmd.patch
patch -f -p1 < ../../patches/s3cmd_2.0.2_max_retries.patch

cd ..
