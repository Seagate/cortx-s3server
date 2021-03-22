#!/usr/bin/env python3

import sys
import json
from plumbum import local

key = sys.argv[1]
nr = int(sys.argv[2]) if len(sys.argv) > 2 else 3
bucket = "test"
body = "/tmp/test"
body_last = "/tmp/test_last"
output = "/tmp/get"
s3api = local["aws"]["s3api"]
create_multipart = json.loads(s3api["create-multipart-upload", "--bucket", bucket, "--key", key]())
upload_id = create_multipart["UploadId"]
print(create_multipart)
parts = [s3api["upload-part", "--bucket", bucket, "--key", key, "--part-number",
               str(i+1), "--upload-id", upload_id,
               "--body", body if i != nr - 1 else body_last]()
         for i in range(nr)]
print(parts)
list_parts = json.loads(s3api["list-parts", "--bucket", bucket, "--key", key,
                              "--upload-id", upload_id]())
print(list_parts)
parts = {}
parts["Parts"] = [{k: v for k, v in d.items() if k in ["ETag", "PartNumber"]}
                  for d in list_parts["Parts"]]
print(parts)
with open('/tmp/parts.json', 'w') as f:
    f.write(json.dumps(parts))
print(s3api["complete-multipart-upload", "--multipart-upload",
            "file:///tmp/parts.json", "--bucket", bucket, "--key", key,
            "--upload-id", upload_id]())
s3api["get-object",  "--bucket", bucket, "--key", key, output]()
