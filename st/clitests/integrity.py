#!/usr/bin/env python3

import sys
import json
import uuid
import random
import argparse

import plumbum
from plumbum import local
from typing import List, Optional


s3api = local["aws"]["s3api"]
OBJECT_SIZE = [0, 1, 2, 4095, 4096, 4097,
               2**20 - 1, 2**20, 2**20 + 100, 2 ** 24]
PART_SIZE = [5 * 2 ** 20, 6 * 2 ** 20]
PART_NR = [i+1 for i in range(2)]
CORRUPTIONS = {'none': 'k', 'zero': 'z', 'first_byte': 'f'}


def create_random_file(path: str, size: int, first_byte: str):
    print(f'create_random_file path={path} size={size} '
          f'first_byte={first_byte}')
    with open('/dev/urandom', 'rb') as r:
        with open(path, 'wb') as f:
            data = r.read(size)
            if size > 0:
                ba = bytearray(data)
                ba[0] = ord(first_byte[0])
                data = bytes(ba)
            f.write(data)


def test_get(bucket: str, key: str, output: str, get_must_fail: bool):
    try:
        local['rm']['-vf', output]
        s3api["get-object", "--bucket", bucket, "--key", key, output]()
        assert not get_must_fail
    except plumbum.commands.processes.ProcessExecutionError as e:
        print(e)
        assert get_must_fail


def test_multipart_upload(bucket: str, key: str, output: str,
                          parts: List[str], get_must_fail: bool) -> None:
    create_multipart = json.loads(s3api["create-multipart-upload",
                                        "--bucket", bucket, "--key", key]())
    upload_id = create_multipart["UploadId"]
    print(create_multipart)
    i: int = 1
    for part in parts:
        print(s3api["upload-part", "--bucket", bucket, "--key", key,
                    "--part-number", str(i), "--upload-id", upload_id,
                    "--body", part]())
        i += 1
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
    test_get(bucket, key, output, get_must_fail)
    if not get_must_fail:
        (local['cat'][parts] |
         local['diff']['--report-identical-files', '-', output])()
    s3api["delete-object",  "--bucket", bucket, "--key", key]()
    

def test_put_get(bucket: str, key: str, body: str, output: str,
                 get_must_fail: bool) -> None:
    s3api["put-object",  "--bucket", bucket, "--key", key, '--body', body]()
    test_get(bucket, key, output, get_must_fail)
    if not get_must_fail:
        (local['diff']['--report-identical-files', body, output])()
    s3api["delete-object",  "--bucket", bucket, "--key", key]()
    

def auto_test_put_get(args) -> None:
    first_byte = CORRUPTIONS[args.corruption]
    for i in range(args.iterations):
        print(f'iteration {i}...')
        for size in OBJECT_SIZE:
            if args.create_objects:
                create_random_file(args.body, size, first_byte)
            test_put_get(args.bucket, f'size={size}_i={i}',
                         args.body, args.output,
                         size > 0 and args.corruption != 'none')
    print('auto-test-put-get: Successful.')


def auto_test_multipart(args) -> None:
    first_byte = CORRUPTIONS[args.corruption]
    for part_size in PART_SIZE:
        for last_part_size in OBJECT_SIZE:
            for part_nr in PART_NR:
                parts = [f'{args.body}.part{i+1}' for i in range(part_nr)]
                corrupted = random.randrange(len(parts) + (last_part_size > 0))
                for i, part in enumerate(parts):
                    if args.create_objects:
                        create_random_file(part, part_size, first_byte
                                           if i == corrupted else 'k')
                if last_part_size > 0:
                    parts += [f'{args.body}.last_part']
                    if args.create_objects:
                        create_random_file(parts[-1], last_part_size,
                                           first_byte
                                           if corrupted == len(parts) - 1
                                           else 'k')
                test_multipart_upload(args.bucket,
                                      f'part_size={part_size}_'
                                      f'last_part_size={last_part_size}_'
                                      f'part_nr={part_nr}_'
                                      f'uuid={uuid.uuid4()}',
                                      args.output, parts,
                                      args.corruption != 'none')
    print('auto-test-multipart: Successful.')


def main() -> None:
    random.seed('integrity')
    parser = argparse.ArgumentParser()
    parser.add_argument('--auto-test-put-get', action='store_true')
    parser.add_argument('--auto-test-multipart', action='store_true')
    parser.add_argument('--auto-test-all', action='store_true')
    parser.add_argument('--bucket', type=str, default='test')
    parser.add_argument('--object-size', type=int, default=2 ** 20)
    parser.add_argument('--body', type=str, default='./s3-object.bin')
    parser.add_argument('--output', type=str, default='./s3-object-output.bin')
    parser.add_argument('--iterations', type=int, default=1)
    parser.add_argument('--create-objects', action='store_true')
    parser.add_argument('--corruption', choices=CORRUPTIONS.keys(),
                        default='none')
    args = parser.parse_args()
    print(args)
    if args.auto_test_put_get:
        auto_test_put_get(args)
    if args.auto_test_multipart:
        auto_test_multipart(args)
    if args.auto_test_all:
        args.create_objects = True
        for args.corruption in CORRUPTIONS.keys():
            auto_test_put_get(args)
            auto_test_multipart(args)
        print('auto-test-all: Successful.')


if __name__ == '__main__':
    main()
