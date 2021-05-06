#!/usr/bin/env python3

import json
import uuid
import random
import string
import argparse
from os import path

from plumbum import local
from typing import List

from awss3api import AwsTest


OBJECT_SIZE = [0, 1, 4097, 2**20 - 1, 2**20 + 100]
PART_SIZE = [5 * 2 ** 20]
PART_NR = [i+1 for i in range(2)]
CORRUPTIONS = {'none-on-write': 'k', 'zero-on-write': 'z',
               'first_byte-on-write': 'f', 'none-on-read': 'K',
               'zero-on-read': 'Z', 'first_byte-on-read': 'F'}


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
    local['rm']['-vf', output]()
    s3api_res = AwsTest('get-object').with_cli_self(f'aws s3api get-object --bucket "{bucket}" --key "{key}" {output}').execute_test(negative_case=get_must_fail)
    if get_must_fail:
        s3api_res.command_should_fail()
    else:
        s3api_res.command_is_successful()


def test_multipart_upload(bucket: str, key: str, output: str,
                          parts: List[str], get_must_fail: bool) -> None:
    s3api_res = AwsTest('create-multipart-upload').with_cli_self(f'aws s3api --output json create-multipart-upload --bucket "{bucket}" --key "{key}"').execute_test()
    create_multipart = json.loads(s3api_res.status.stdout)
    upload_id = create_multipart["UploadId"]
    print(create_multipart)
    i: int = 1
    for part in parts:
        s3api_res = AwsTest('upload-part').with_cli_self(f'aws s3api upload-part --bucket "{bucket}" --key "{key}" --part-number "{i}" --upload-id "{upload_id}" --body "{part}"').execute_test()
        print(s3api_res.status.stdout)
        i += 1

    s3api_res = AwsTest('list-parts').with_cli_self(f'aws s3api list-parts --output json --bucket "{bucket}" --key "{key}" --upload-id "{upload_id}"').execute_test()
    parts_list = json.loads(s3api_res.status.stdout)
    print(parts_list)
    parts_file = {}
    parts_file["Parts"] = [{k: v for k, v in d.items()
                            if k in ["ETag", "PartNumber"]}
                           for d in parts_list["Parts"]]
    print(parts_file)
    part_json = path.abspath('./parts.json')
    with open(part_json, 'w') as f:
        f.write(json.dumps(parts_file))

    s3api_res = AwsTest('complete-multipart-upload').with_cli_self(f'aws s3api complete-multipart-upload --multipart-upload "file://{part_json}" --bucket "{bucket}" --key "{key}" --upload-id "{upload_id}"').execute_test()
    print(s3api_res.status.stdout)
    test_get(bucket, key, output, get_must_fail)
    if not get_must_fail:
        (local['cat'][parts] |
         local['diff']['--report-identical-files', '-', output])()
    AwsTest('delete-object').with_cli_self("aws s3api delete-object --bucket " + bucket + " --key " + key).execute_test().command_is_successful()


def test_put_get(bucket: str, key: str, body: str, output: str,
                 get_must_fail: bool) -> None:
    AwsTest('test_put_get').with_cli_self(f'aws s3api put-object --bucket "{bucket}" --key "{key}" --body "{body}"').execute_test().command_is_successful()
    test_get(bucket, key, output, get_must_fail)
    if not get_must_fail:
        (local['diff']['--report-identical-files', body, output])()
    AwsTest('delete-object').with_cli_self("aws s3api delete-object --bucket " + bucket + " --key " + key).execute_test().command_is_successful()


def auto_test_put_get(args, object_size: List[int]) -> None:
    first_byte = CORRUPTIONS[args.corruption]
    for i in range(args.iterations):
        print(f'iteration {i}...')
        for size in object_size:
            if args.create_objects:
                create_random_file(args.body, size, first_byte)
            test_put_get(args.bucket, f'size={size}_i={i}',
                         args.body, args.output,
                         size > 0 and
                         not args.corruption. startswith('none-on-'))
    print('auto-test-put-get: Successful.')


def auto_test_multipart(args) -> None:
    first_byte = CORRUPTIONS[args.corruption]
    for part_size in PART_SIZE:
        for last_part_size in OBJECT_SIZE:
            for part_nr in PART_NR:
                parts = [f'{args.body}.part{i+1}' for i in range(part_nr)]
                # random is used here intentionally. We don't need
                # cryptographic-level randomness here.
                # random allows us to make tests reproducible by providing the
                # same test sequence.
                corrupted = 0 if first_byte in ['Z', 'F'] else \
                    random.randrange(len(parts) + (last_part_size > 0))
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
                                      not args.corruption.
                                      startswith('none-on-'))
    print('auto-test-multipart: Successful.')


def main() -> None:
    random.seed('integrity')
    parser = argparse.ArgumentParser()
    parser.add_argument('--auto-test-put-get', action='store_true')
    parser.add_argument('--auto-test-multipart', action='store_true')
    parser.add_argument('--test-put-get', action='store_true')
    parser.add_argument('--auto-test-all', action='store_true')
    parser.add_argument('--bucket', type=str, default='')
    parser.add_argument('--object-size', type=int, default=2 ** 20)
    parser.add_argument('--body', type=str, default='./s3-object.bin')
    parser.add_argument('--output', type=str, default='./s3-object-output.bin')
    parser.add_argument('--iterations', type=int, default=1)
    parser.add_argument('--create-objects', action='store_true')
    parser.add_argument('--corruption', choices=CORRUPTIONS.keys(),
                        default='none-on-write')
    args = parser.parse_args()
    print(args)
    args.body = path.abspath(args.body)
    args.output = path.abspath(args.output)
    if not args.bucket:
        random.seed(a=None)
        allsym = f"{string.ascii_lowercase}{string.digits}-"
        args.bucket = "{}{}{}".format(
            random.choice(string.ascii_lowercase),
            ''.join(random.choice(allsym) for i in range(20)),
            random.choice(string.digits))
    AwsTest('s3 mb').with_cli_self(f"aws s3 mb s3://{args.bucket}").execute_test().command_is_successful()
    if args.test_put_get:
        auto_test_put_get(args, [args.object_size])
    if args.auto_test_put_get:
        auto_test_put_get(args, OBJECT_SIZE)
    elif args.auto_test_multipart:
        auto_test_multipart(args)
    elif args.auto_test_all:
        args.create_objects = True
        for args.corruption in CORRUPTIONS.keys():
            auto_test_put_get(args, OBJECT_SIZE)
            auto_test_multipart(args)
        print('auto-test-all: Successful.')


if __name__ == '__main__':
    main()
