### License

Copyright (c) 2021 Seagate Technology LLC and/or its Affiliates

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

For any questions about this software or licensing,
please email opensource@seagate.com or cortx-questions@seagate.com.

# Object Integrity testing

- Ensure config paramter `S3_READ_MD5_CHECK_ENABLED` is set to `true`
- Enable Fault Injection
- Run test workload either automatically or manually

## Enable `Fault Injection`

Ensure s3server is run with command line parameter `--fault_injection true`

- On dev VM run s3server with `dev-starts3.sh`
- On prod deployment:
    - stop cluster
    - edit `/opt/seagate/cortx/s3/s3startsystem.sh` on each cluster's node
      by adding `--fault_injection true` to the s3server start command
    - start cluster

Specific fault injection could be enabled or disabled by the following command

```
# curl -X PUT -H "x-seagate-faultinjection: enable,always,<fi_tag>,0,0" <s3_instance>
```

where

- `<fi_tag>` - name of particular fault injection, could be one of:

  - `di_data_corrupted_on_write` - corrupts file before storing;
  - `di_data_corrupted_on_read` - corrupts file during retrieval;
  - `di_obj_md5_corrupted` - instead of md5 hash of the object stores
    dm5 hash of empty string.

- `<s3_instance>` - particular s3server instance to enable fault injection on

### FI example commands

- Enable file corruption for a single instance of s3server

```
# curl -X PUT -H "x-seagate-faultinjection: enable,always,di_data_corrupted_on_write,0,0" localhost:28081
```

- Disable file corruption for a single instance of s3server

```
# curl -X PUT -H "x-seagate-faultinjection: disable,always,di_data_corrupted_on_write,0,0" localhost:28081
```

- Test if FI file corruption is enabled for a single instance of s3server.
  Following command prints FI status to s3server instance's log file

```
# curl -X PUT -H "x-seagate-faultinjection: test,always,di_data_corrupted_on_write,0,0" localhost:28081
```

### Helper script for edit startup command line flags

```
#!/usr/bin/env bash

[ -z "$NODES" ] && { echo "Provide NODES"; exit 1; }
s3cmdup=${S3STARTUP:-"/opt/seagate/cortx/s3/s3startsystem.sh"}
if [ "$1" = "enable" ]; then
    echo "Enabling"
    pdsh -S -w $NODES "sed -i \"s/s3server[[:space:]]*--/s3server --fault_injection true --/g\" $s3cmdup"
    echo "Done"
fi
if [ "$1" = "disable" ]; then
    echo "Disabling"
    pdsh -S -w $NODES "sed -i \"s/[[:space:]]*--fault_injection true//g\" $s3cmdup"
    echo "Done"
fi
```

Params:

- `NODES` - comma separated list of node's names
- `S3STARTUP` - path to s3server starting script
- command as a positional agr

Save script as `cmd_s3_startup.sh`.

- Add `--fault_injection true` to all startup commands on all cluster's nodes

```
# NODES="node1,node2" ./cmd_s3_startup.sh enable
```

- Remove `--fault_injection true` from all startup commands on all cluster's nodes

```
# NODES="node1,node2" ./cmd_s3_startup.sh disable
```

By default script will look for a s3server's startup script in
`/opt/seagate/cortx/s3/s3startsystem.sh`. It could be changed with
`S3STARTUP` script's parameter.

### Helper script for enabling and disabling Fault Injections

```
#!/usr/bin/env bash

[ -z "$NODES" ] && { echo "Provide NODES"; exit 1; }
[ -z "$FIS" ] && { echo "Provide FIS"; exit 1; }
[ -z "$1" ] && { echo "Operation missed"; exit 1; }
inst_num=${NINST:-"45"}
start_port=28081
end_port=$(( $start_port + $inst_num - 1 ))
for f in $(echo $FIS | tr "," " ")
do
    sgtfi="x-seagate-faultinjection: $1,always,$f,0,0"
    for port in $(seq $start_port $end_port | tr "\n" " ")
    do
        pdsh -S -w  $NODES "curl -X PUT -H \"$sgtfi\" localhost:$port"
    done
done
```

Params:

- `NODES` - comma separated list of node's names
- `FIS` - comma separated list fault injection names
- `NINST` - number of s3server instances run on a node
- command as a positional agr

Save script as `cmd_fi_s3.sh`.

- Enable `di_data_corrupted_on_write` on s3server instances on cluster's nodes

```
# NINST=32 NODES="node1,node2" FIS="di_data_corrupted_on_write" ./cmd_fi_s3.sh enable
```

- Enable several fault injections on s3server instances on cluster's nodes

```
# NINST=32 NODES="node1,node2" FIS="di_data_corrupted_on_write,di_obj_md5_corrupted" ./cmd_fi_s3.sh enable
```

- Disable fault injections on s3server instances on cluster's nodes

```
# NINST=32 NODES="node1,node2" FIS="di_data_corrupted_on_write,di_obj_md5_corrupted" ./cmd_fi_s3.sh disable
```

- Test fault injections on s3server instances on cluster's nodes

```
# NINST=32 NODES="node1,node2" FIS="di_data_corrupted_on_write,di_obj_md5_corrupted" ./cmd_fi_s3.sh test
```

## Automatically

Enable fault injection for data, i.e. `di_data_corrupted_on_write`, and then

    time st/clitests/integrity.py --auto-test-all

`aws s3` and `aws s3api` must be configured and must work.

## Manually

### Data path

If the first byte of the object is one of the following values and relevant
fault injection is enabled then the object may be corrupted.

- `di_data_corrupted_on_write`:
  - 'k' then the file is not corrupted during `put-object` and `upload-part`;
  - 'z' then it's zeroed during upload after calculating checksum, but before
    sending data to Motr;
  - 'f' then the first byte of the object is set to 0 like it's done for 'z'
    case;
- `di_data_corrupted_on_read`:
  - 'K' then the file is not corrupted during `get-object`;
  - 'Z' then it's zeroed during `get-object` just after reading data from Motr,
    but before calculating checksum;
  - 'F' then the first byte is set to 0 during `get-object` like it's done for
    'Z' case.

#### PUT/GET

- create a file `dd if=/dev/urandom of=./s3-object.bin bs=1M count=1`, set the
  size as needed;

    aws s3api put-object --bucket test --key object_key --body ./s3-object.bin
    aws s3api get-object --bucket test --key object_key ./s3-object-get.bin
    diff --report-identical-files ./s3-object ./s3-object-get.bin

#### Multipart

    s3api create-multipart-upload --bucket test --key test00
    s3api upload-part --bucket test --key test00 --upload-id 9db1fffe-0879-4113-a85d-9967481dda3c --part-number 1 --body file.42
    s3api upload-part --bucket test --key test00 --upload-id 9db1fffe-0879-4113-a85d-9967481dda3c --part-number 2 --body file.1
    s3api complete-multipart-upload --multipart-upload file://./parts.json --bucket test --key test00 --upload-id 9db1fffe-0879-4113-a85d-9967481dda3c
    s3api get-object --bucket test --key test00 ./s3-object-output.bin

- take `upload_id` from `create-multipart-upload output;
- ./parts.json must be like this, `Etag` and `PartNumber` must be taken from
  `upload-part` output:

      {
          "Parts": [
              {
                  "PartNumber": 1,
                  "ETag": "\"72f0bd8323296e0c8706c085e8d1ff00\""
              },
              {
                  "PartNumber": 2,
                  "ETag": "\"4905c7a920af8680d07512f336999583\""
              }
          ]
      }

### Metadata path

Enable fault injection for data, i.e. `di_obj_md5_corrupted`.
Any file used for PUT/GET will generate `Content checksum mismatch` error

# Metadata Integrity testing

## Manual metadata integrity testing

- Enable Fault Injection as described in [Enable `Fault Injection`](#enable-fault-injection)
- Run test workload

### Supported fault injections

- `di_metadata_bcktname_on_write_corrupted` - corrupts bucket name in object metadata on writing to store
- `di_metadata_objname_on_write_corrupted` - corrupts object name in object metadata on writing to store
- `di_metadata_bcktname_on_read_corrupted` - corrupts bucket name in object metadata on reading from store
- `di_metadata_objname_on_read_corrupted` - corrupts object name in object metadata on reading from store
- `object_metadata_corrupted` - object metadata could not be parsed
- `di_metadata_bucket_or_object_corrupted` - object and bucket names could not be validated against request data
- `part_metadata_corrupted` - part metadata could not be parsed
- `di_part_metadata_bcktname_on_write_corrupted` - corrupts bucket name in part metadata on writing to store
- `di_part_metadata_objname_on_write_corrupted` - corrupts object name in part metadata on writing to store
- `di_part_metadata_bcktname_on_read_corrupted` - corrupts bucket name in part metadata on reading from store
- `di_part_metadata_objname_on_read_corrupted` - corrupts object name in part metadata on reading from store

### Workload for manual testing

Data used in testing does not require any special preparations - any files could be used.

## Automated metadata integrity testing

`st/clitests/md_integrity.md` script were created to do automated test.

The script can run predefined test plans, which could be generated in form of json.
`md_integrity.md` uses `aws s3api` to send requests, so it should be installed and
properly configured.

Test plan supports following operations

- `aws s3api` commands
    - create-bucket
    - delete-bucket
    - put-object
    - get-object
    - head-object
    - delete-object
    - create-multipart
    - upload-part
    - complete-multipart
    - list-parts

- `fault injection` commands
    - enable-fi
    - disable-fi

Description and parameters for the commands from `aws s3api` group could be
obtained with `aws s3api <command> help` shell command.

`enable-fi` - enables specified fault injection. Command has a single parameter named
`fi` which is the name of the fault injection to enable.

`disable-fi` - disables specified fault injection. Command has a single parameter named
`fi` which is the name of the fault injection to disable.

### Test plan description

Test plan is a simple json file with the following structure

```
{
    "desc": "Plan Description",
    "tests": [...]
}
```

- `desc` - optional - Test plan description
- `tests` - mandatory - List of commands to execute. All the commands are run
  sequentially in order of description.

Each command has the following structure

```
{
    "desc": "Command description",
    "expect": true,
    "op": <command-name>,
    <command specific params>
    ...
}
```

- `desc` - optional - Command description.
- `expect` - optional - Boolean value representing expectation about command status:
  `true` - command should be succesful, `false` - command should fail.
      If omitted `true` is assumed.
- `op` - mandatory - Name of the command to perform. Should be from the list of
  supported commands
- `<command specific params>` - arguments specific to command.

Examples

```
{"desc": "Create bucket and PUT one object, after that delete all. All command are expected to run ok.",
 "tests": [
     {"desc": "Create bucket test-bucket-1",
      "op": "create-bucket", "bucket": "test-bucket-1", "expect": true},
     {"desc": "Put object object1 to bucket test-bucket-1",
      "op": "put-object", "bucket": "test-bucket-1", "key": "object1", "body": "/tmp/data.bin", "expect": true},
     {"desc": "Delete-object request for previous object",
      "op": "delete-object", "bucket": "test-bucket-1", "key": "object1", "expect": true},
     {"desc": "Delet bucket test-bucket-1",
      "op": "delete-bucket", "bucket": "test-bucket-1", "expect": true}
 ]}
```

It is not neccessary to enable any fault injections manualy. It could be done
in test plan

```
{"desc": "Create bucket, put object, enable FI, try to get, expect FAIL, disable FI, try to get, expect ok",
 "tests": [
     {"desc": "Create bucket test-bucket-1",
      "op": "create-bucket", "bucket": "test-bucket-1", "expect": true},
     {"desc": "Put object object1 to bucket test-bucket-1",
      "op": "put-object", "bucket": "test-bucket-1", "key": "object1", "body": "/tmp/data.bin", "expect": true},
     {"desc": "Enable FI di_metadata_bucket_or_object_corrupted",
      "op": "enable-fi", "fi": "di_metadata_bucket_or_object_corrupted"},
     {"desc": "Get-object request for previous object",
      "op": "get-object", "bucket": "test-bucket-1", "key": "object1", "expect": false},
     {"desc": "Disable FI di_metadata_bucket_or_object_corrupted",
      "op": "disable-fi", "fi": "di_metadata_bucket_or_object_corrupted"},
     {"desc": "Get-object request for previous object",
      "op": "get-object", "bucket": "test-bucket-1", "key": "object1", "expect": true},
     {"desc": "Delete-object request for previous object",
      "op": "delete-object", "bucket": "test-bucket-1", "key": "object1", "expect": true},
     {"desc": "Delet bucket test-bucket-1",
      "op": "delete-bucket", "bucket": "test-bucket-1", "expect": true}
 ]}
```
### Script parameters

- Mandatory parameters
    - `--test_plan` - Path to a json file with Test plan description
    
- Optional paramters
    - `--download` - Path to temp location where to store downloaded file
    - `--parts` - Path to temp location where to store part description for multipart upload
    - Following params could be set via cmd to simplify Test plan
        - `--bucket` - Bucket name that will be used in all commands if not redefined in Test plan.
          If empty value provided bucket name will be generated randomly.
        - `--key` - Key that will be used in all commands if not redefined in Test plan.
        - `--body` - Body that will be used in all commands if not redefined in Test plan.

Examples

```
./md_integrity.md --test_plan ./plan.json
```

```
./md_integrity.md --test_plan ./plan.json --body ./data.bin
```

### Predefined Test plans

There several predefined tests plans used in system tests

- `st/clitests/multipart_md_integrity.json`
- `st/clitests/regular_md_integrity.json`

### `jenkins-build`

System tests for Metadata integrity are run from `jenkins_build`

```
# Metada Integrity tests - regular PUT
md_di_data=/tmp/s3-data.bin
md_di_dowload=/tmp/s3-data-download.bin
md_di_parts=/tmp/s3-data-parts.json

$USE_SUDO dd if=/dev/urandom of=$md_di_data count=1 bs=1K
$USE_SUDO ./md_integrity.py --body $md_di_data --download $md_di_dowload --parts $md_di_parts --test_plan ./regular_md_integrity.json

# Metada Integrity tests - multipart
$USE_SUDO dd if=/dev/urandom of=$md_di_data count=1 bs=5M
$USE_SUDO ./md_integrity.py --body $md_di_data --download $md_di_dowload --parts $md_di_parts --test_plan ./metadata_md_integrity.json

[ -f $md_di_data ] && $USE_SUDO rm -vf $md_di_data
[ -f $md_di_dowload ] && $USE_SUDO rm -vf $md_di_dowload
[ -f $md_di_parts ] && $USE_SUDO rm -vf $md_di_parts
# ======================================
```
