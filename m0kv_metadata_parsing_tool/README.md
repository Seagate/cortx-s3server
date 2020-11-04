# m0kv_metadata_parsing_tool
This tool can parse all metadata for all buckets and objects with m0kv.

## License
```
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
```

## Installation
Clone the tool from cortx-s3server

## Requirements
Requires m0kv binary.
*   run build_motr.sh in 'third_party/build_motr.sh'
*   m0kv binary is generated here 'third_party/motr/utils/m0kv'

## Usage
~~~
Usage: ./m0kv_metadata_parsing_tool -l 'local_addr' -h 'ha_addr' -p 'profile' -f 'proc_fid' -d 'depth' -b 'bucket_name'
for eg.
./m0kv_metadata_parsing_tool -l 10.230.247.176@tcp:12345:33:905 -h 10.230.247.176@tcp:12345:34:1 -p '<0x7000000000000001:0>' -f '<0x7200000000000000:0>' -d 1
-----------------------------------------------------------------------------------
-d represents depth of traversal
-d 1  -> Get metadata of only buckets
-d >1 -> Gets metadata of bucket and object
-----------------------------------------------------------------------------------
-b <bucket_name> is an optional arguement to get metadata specific to bucket
~~~

## Example 
~~~
./m0kv_metadata_parsing_tool.sh -l 10.230.247.176@tcp:12345:33:905 -h 10.230.247.176@tcp:12345:34:1 -p '<0x7000000000000001:0>' -f '<0x7200000000000000:0>' -d 1 -b test
cat: third_party/motr/motr/m0kv: Is a directory
Metadata of test
##########################################################################################
bucketname : test
motr_multipart_index_oid : 0x7800000000ac0a74:0xe7240000000004ad
motr_object_list_index_oid : 0x7800000000ac0a74:0xe7240000000004ac
motr_objects_version_list_index_oid : 0x7800000000ac0a74:0xe7240000000004ae
##########################################################################################
./m0kv_metadata_parsing_tool.sh: line 78: ./third_party/motr/utils/m0kv -l 10.230.247.176@tcp:12345:33:905 -h 10.230.247.176@tcp:12345:34:1 -p <0x7000000000000001:0> -f <0x7200000000000000:0> index next 0x7800000000000000:0x100003 '0' 100 -s: No such file or directory
##########################################################################################
Probable Delete List Index
##########################################################################################
##########################################################################################
Log file : /var/log/m0kv_metadata.log
[root@ssc-vm-1177 m0kv_metadata_parsing_tool]# > m0kv_metadata_parsing_tool.sh
[root@ssc-vm-1177 m0kv_metadata_parsing_tool]# nano m0kv_metadata_parsing_tool.sh
[root@ssc-vm-1177 m0kv_metadata_parsing_tool]# ./m0kv_metadata_parsing_tool.sh -l 10.230.247.176@tcp:12345:33:905 -h 10.230.247.176@tcp:12345:34:1 -p '<0x7000000000000001:0>' -f '<0x7200000000000000:0>' -d 1 -b test
cat: third_party/motr/motr/m0kv: Is a directory
Metadata of test
##########################################################################################
bucketname : test
motr_multipart_index_oid : 0x7800000000ac0a74:0xe7240000000004ad
motr_object_list_index_oid : 0x7800000000ac0a74:0xe7240000000004ac
motr_objects_version_list_index_oid : 0x7800000000ac0a74:0xe7240000000004ae
##########################################################################################
##########################################################################################
Probable Delete List Index
##########################################################################################
key : 0xac0a74:0xe7240000000005d7
objectname : file.txt
object_list_index_oid : 0x7800000000ac0a74:0xe7240000000004c7
object_version_list_index_oid : 0x7800000000ac0a74:0xe7240000000004c8
part_list_idx_oid : 0x7800000000ac0a74:0xe7240000000005d9
key : 0xac0a74:0xe7240000000005f4
objectname : file.txt
object_list_index_oid : 0x7800000000ac0a74:0xe7240000000004c7
object_version_list_index_oid : 0x7800000000ac0a74:0xe7240000000004c8
part_list_idx_oid : 0x7800000000ac0a74:0xe7240000000005f6
key : 0xac0a74:0xe72400000000061c
objectname : file.txt
object_list_index_oid : 0x7800000000ac0a74:0xe7240000000004c7
object_version_list_index_oid : 0x7800000000ac0a74:0xe7240000000004c8
part_list_idx_oid : 0x7800000000ac0a74:0xe72400000000061e
key : 0xac0a74:0xe724000000000640
objectname : file.txt
object_list_index_oid : 0x7800000000ac0a74:0xe7240000000004c7
object_version_list_index_oid : 0x7800000000ac0a74:0xe7240000000004c8
part_list_idx_oid : 0x7800000000ac0a74:0xe724000000000642
key : 0xac0a74:0xe724000000000541
objectname : file.txt
object_list_index_oid : 0x7800000000ac0a74:0xe7240000000004c7
object_version_list_index_oid : 0x7800000000ac0a74:0xe7240000000004c8
part_list_idx_oid : 0x7800000000ac0a74:0xe724000000000543
key : 0xac0a74:0xe724000000000665
objectname : file1.txt
object_list_index_oid : 0x7800000000ac0a74:0xe7240000000004c7
object_version_list_index_oid : 0x7800000000ac0a74:0xe7240000000004c8
part_list_idx_oid : 0x7800000000ac0a74:0xe724000000000667
key : 0xac0a74:0xe724000000000686
objectname : file1.txt
object_list_index_oid : 0x7800000000ac0a74:0xe7240000000004c7
object_version_list_index_oid : 0x7800000000ac0a74:0xe7240000000004c8
part_list_idx_oid : 0x7800000000ac0a74:0xe724000000000688
key : 0xac0a74:0xe724000000000591
objectname : file.txt
object_list_index_oid : 0x7800000000ac0a74:0xe7240000000004c7
object_version_list_index_oid : 0x7800000000ac0a74:0xe7240000000004c8
part_list_idx_oid : 0x7800000000ac0a74:0xe724000000000593
key : 0xac0a74:0xe7240000000006a3
objectname : file1.txt
object_list_index_oid : 0x7800000000ac0a74:0xe7240000000004c7
object_version_list_index_oid : 0x7800000000ac0a74:0xe7240000000004c8
part_list_idx_oid : 0x7800000000ac0a74:0xe7240000000006a5
key : 0xac0a74:0xe7240000000005b3
objectname : file.txt
object_list_index_oid : 0x7800000000ac0a74:0xe7240000000004c7
object_version_list_index_oid : 0x7800000000ac0a74:0xe7240000000004c8
part_list_idx_oid : 0x7800000000ac0a74:0xe7240000000005b5
key : 0xac0a74:0xe7240000000004c9
objectname : file.txt
object_list_index_oid : 0x7800000000ac0a74:0xe7240000000004c7
object_version_list_index_oid : 0x7800000000ac0a74:0xe7240000000004c8
part_list_idx_oid : 0x7800000000ac0a74:0xe7240000000004cb
##########################################################################################
Log file : /var/log/m0kv_metadata.log

~~~
