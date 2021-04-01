# Object Integrity testing

## Automatically

Set `S3_ST_MD5_ERROR_INJECT_ENABLED: true` and then

    aws s3 mb s3://test
    time st/clitests/integrity.py --auto-test-all

`aws s3` and `aws s3api` must be configured and must work.

## Manually

In `S3_ST_MD5_ERROR_INJECT_ENABLED: true` mode: if the first byte of the file
is:

- 'k' then the file is not corrupted;
- 'z' then it's zeroed during upload after calculating checksum, but before
  sending data to Motr;
- 'f' then the first byte of the object is set to t like for 'z' case.

### PUT/GET

- create a file `dd if=/dev/urandom of=./s3-object.bin bs=1M count=1`, set the
  size as needed;

    aws s3api put-object --bucket test --key object_key --body ./s3-object.bin
    aws s3api get-object --bucket test --key object_key ./s3-object-get.bin
    diff --report-identical-files ./s3-object ./s3-object-get.bin

### Multipart

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
