# License
```#
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
```
# m0kv_metadata_parsing_tool

This tool can parse all metadata for all buckets and objects with m0kv. 
## Installation

Clone the tool from cortx-s3server

## Requirements
1. Requires m0kv binary. 
* run build_motr.sh in 'third_party/build_motr.sh'
* m0kv binary is generated here 'third_party/motr/utils/m0kv'



## Usage

```
Usage: ./m0kv_metadata_parsing_tool -l 'local_addr' -h 'ha_addr' -p 'profile' -f 'proc_fid' -d 'depth' -b 'bucket_name'
for eg.
./m0kv_metadata_parsing_tool -l 10.230.247.176@tcp:12345:33:905 -h 10.230.247.176@tcp:12345:34:1 -p '<0x7000000000000001:0>' -f '<0x7200000000000000:0>' -d 1
-----------------------------------------------------------------------------------
-d represents depth of traversal
-d 1  -> Get metadata of only buckets
-d >1 -> Gets metadata of bucket and object
-----------------------------------------------------------------------------------
-b <bucket_name> is an optional arguement to get metadata specific to bucket
```


