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
./m0kv_metadata_parsing_tool.sh -l 10.230.247.176@tcp:12345:33:905 -h 10.230.247.176@tcp:12345:34:1 -p '<0x7000000000000001:0>' -f '<0x7200000000000000:0>' -b 0 -d 10
-----------------------------------------------------------------------------------
-b is flag and required arguement (can be set 0 or 1)
-b 0 -> Get metadata of bucket and objects
-b 1 -> Get only bucket level metadata
-----------------------------------------------------------------------------------
-d is number of entries for probable delete index (should be integer val)
-d is required arguement
-----------------------------------------------------------------------------------
-n is Optional arguement to get metadata specific to bucket
```

