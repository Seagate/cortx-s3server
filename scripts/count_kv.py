#!/usr/bin/python3.6
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

"""Script to count no of KV's in an index"""

import sys
from s3backgrounddelete.cortx_s3_count_kv import CORTXS3Countkv

if __name__ == "__main__":
    obj = CORTXS3Countkv()
    if len(sys.argv) == 2:
        obj.count(sys.argv[1])
    elif len(sys.argv) > 2:
        obj.count(sys.argv[1], sys.argv[2])
    else:
        obj.count()
