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

- `<fi_tag>` - name of particular fault injection, could be one of
    - `di_data_corrupted` - currupts file before storing
    - `di_obj_md5_corrupted` - instead of md5 hash of the object stores
      dm5 hash of emtpy string

- `<s3_instance>` - particular s3server instance to enable fault injection on

### FI example commands

- Enable file corruption for a single instance of s3server

```
# curl -X PUT -H "x-seagate-faultinjection: enable,always,di_data_corrupted,0,0" localhost:28081
```

- Disable file corruption for a single instance of s3server

```
# curl -X PUT -H "x-seagate-faultinjection: disable,always,di_data_corrupted,0,0" localhost:28081
```

- Test if FI file corruption is enabled for a single instance of s3server.
  Following command prints FI status to s3server instace's log file

```
# curl -X PUT -H "x-seagate-faultinjection: test,always,di_data_corrupted,0,0" localhost:28081
```

## Automatically

Enable fault injection for data, i.e. `di_data_corrupted`, and then

    aws s3 mb s3://test
    time st/clitests/integrity.py --auto-test-all

`aws s3` and `aws s3api` must be configured and must work.

## Manually

### Data path

Enable fault injection for data, i.e. `di_data_corrupted`. If the first byte of the file
is:

- 'k' then the file is not corrupted;
- 'z' then it's zeroed during upload after calculating checksum, but before
  sending data to Motr;
- 'f' then the first byte of the object is set to t like for 'z' case.

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
