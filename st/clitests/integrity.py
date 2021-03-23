#!/usr/bin/env python3

import sys
import json
import argparse

from plumbum import local
from typing import List, Optional


s3api = local["aws"]["s3api"]
OBJECT_SIZE = [0, 1, 2, 4095, 4096, 4097,
               2**20 - 1, 2**20, 2**20 + 100, 2 ** 24]
PART_SIZE = [5 * 2 ** 20, 6 * 20 ** 20]
PART_NR = range(4)


def create_random_file(path: str, size: int, first_byte: Optional[str] = None):
    with open('/dev/urandom', 'rb') as r:
        with open(path, 'wb') as f:
            data = r.read(size)
            if first_byte:
                ba = bytearray(data)
                ba[0] = ord(first_byte[0])
                data = bytes(ba)
            f.write(data)

                       
def test_multipart_upload(bucket: str, key: str, output: str,
                          parts: List[str]) -> None:
    create_multipart = json.loads(s3api["create-multipart-upload",
                                        "--bucket", bucket, "--key", key]())
    upload_id = create_multipart["UploadId"]
    print(create_multipart)
    i: int = 1
    for part in parts:
        print(s3api["upload-part", "--bucket", bucket, "--key", key,
                    "--part-number", str(i), "--upload-id", upload_id,
                    "--body", part]())
    parts_list = json.loads(s3api["list-parts",
                                  "--bucket", bucket, "--key", key,
                                  "--upload-id", upload_id]())
    print(parts_list)
    parts_file = {}
    parts_file["Parts"] = [{k: v for k, v in d.items()
                            if k in ["ETag", "PartNumber"]}
                           for d in parts_list["Parts"]]
    print(parts_file)
    with open('/tmp/parts.json', 'w') as f:
        f.write(json.dumps(parts_file))
    print(s3api["complete-multipart-upload", "--multipart-upload",
                "file:///tmp/parts.json", "--bucket", bucket, "--key", key,
                "--upload-id", upload_id]())
    s3api["get-object",  "--bucket", bucket, "--key", key, output]()
    (local['cat'][parts] |
     local['diff']['--report-identical-files', '-', output])()
    s3api["delete-object",  "--bucket", bucket, "--key", key]()
    

def test_put_get(bucket: str, key: str, body: str, output: str) -> None:
    s3api["put-object",  "--bucket", bucket, "--key", key, '--body', body]()
    s3api["get-object",  "--bucket", bucket, "--key", key, output]()
    (local['diff']['--report-identical-files', body, output])()
    s3api["delete-object",  "--bucket", bucket, "--key", key]()
    

def auto_test_put_get(args) -> None:
    if args.create_objects:
        create_random_file(args.body, args.object_size)
    for size in OBJECT_SIZE:
        for i in range(args.iterations):
            test_put_get(args.bucket, str(i), args.body, args.output)


def auto_test_multipart(args) -> None:
    for part_size in PART_SIZE:
        for last_part_size in OBJECT_SIZE:
            for part_nr in PART_NR:
                parts = [f'{args.body}.part{i+1}' for i in range(part_nr)]
                if last_part_size > 0:
                    parts += [f'{args.body}.last_part']
                for part in parts:
                    create_random_file(part, part_size)
                test_multipart_upload(args.bucket,
                                      f'part_size={part_size}_'
                                      f'last_part_size={last_part_size}_'
                                      f'part_nr={part_nr}',
                                      args.output, parts)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument('--auto-test-put-get', action='store_true')
    parser.add_argument('--auto-test-multipart', action='store_true')
    parser.add_argument('--bucket', type=str, default='test')
    parser.add_argument('--object-size', type=int, default=2 ** 20)
    parser.add_argument('--body', type=str, default='./s3-object.bin')
    parser.add_argument('--output', type=str, default='./s3-object-output.bin')
    parser.add_argument('--iterations', type=int, default=1)
    parser.add_argument('--create-objects', action='store_true')
    args = parser.parse_args()
    print(args)
    if args.auto_test_put_get:
        auto_test_put_get(args)
    if args.auto_test_multipart:
        auto_test_multipart(args)


if __name__ == '__main__':
    main()
