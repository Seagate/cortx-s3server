#!/usr/bin/env python3

import json
import random
import string
import argparse
from os import path, system

from s3fi import S3fiTest
from awss3api import AwsTest


def get_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('--bucket', type=str, default='')
    parser.add_argument('--key', type=str, default='')
    argbody = parser.add_argument('--body', type=str, default='')
    parser.add_argument('--download', type=str, default='/tmp/s3-data-from-server.bin')
    parser.add_argument('--parts', type=str, default='/tmp/parts.json')
    argtpl = parser.add_argument('--test_plan', type=str, default='', required=True)

    args = parser.parse_args()

    args.download = path.abspath(args.download)
    args.parts = path.abspath(args.parts)

    if args.body:
        args.body = path.abspath(args.body)
        if not path.isfile(args.body):
            raise argparse.ArgumentError(argbody, "--body must be a path to existing file")

    if args.test_plan:
        args.test_plan = path.abspath(args.test_plan)


    test_plan = {}
    try:
        with open(args.test_plan, 'r') as tp:
            test_plan = json.load(tp)
    except Exception as ex:
        raise argparse.ArgumentError(argtpl, f"--test_plan must be a path to existing json file. {ex}")

    if not args.bucket:
        allsym = f"{string.ascii_lowercase}{string.digits}-"
        args.bucket = "{}{}{}".format(
            random.choice(string.ascii_lowercase),
            ''.join(random.choice(allsym) for i in range(20)),
            random.choice(string.digits))

    if not args.key:
        args.key = ''.join(random.choice(string.ascii_lowercase) for i in range(22))

    return args, test_plan


def test_create_bucket(**kwargs):
    expect = kwargs.get("expect", True)
    desc = kwargs.get("desc", "Test")
    bucket = kwargs["bucket"]
    s3api_res = AwsTest(desc).with_cli_self(f'aws s3api create-bucket --bucket "{bucket}"').execute_test(negative_case=not expect)
    if expect:
        s3api_res.command_is_successful()
    else:
        s3api_res.command_should_fail()


def test_delete_bucket(**kwargs):
    expect = kwargs.get("expect", True)
    desc = kwargs.get("desc", "Test")
    bucket = kwargs["bucket"]
    s3api_res = AwsTest(desc).with_cli_self(f'aws s3api delete-bucket --bucket "{bucket}"').execute_test(negative_case=not expect)
    if expect:
        s3api_res.command_is_successful()
    else:
        s3api_res.command_should_fail()


def test_put(**kwargs):
    expect = kwargs.get("expect", True)
    desc = kwargs.get("desc", "Test")
    key = kwargs["key"]
    body = path.abspath(kwargs["body"])
    bucket = kwargs["bucket"]
    s3api_res = AwsTest(desc).with_cli_self(f'aws s3api put-object --output json --bucket "{bucket}" --key "{key}" --body "{body}"').execute_test(negative_case=not expect)
    if expect:
        s3api_res.command_is_successful()
        print(f"{json.loads(s3api_res.status.stdout)}")
    else:
        s3api_res.command_should_fail()


def test_get(**kwargs):
    expect = kwargs.get("expect", True)
    desc = kwargs.get("desc", "Test")
    bucket = kwargs["bucket"]
    key = kwargs["key"]
    download = path.abspath(kwargs["download"])
    system(f'rm -vf "{download}"')
    s3api_res = AwsTest(desc).with_cli_self(f'aws s3api get-object --bucket "{bucket}" --key "{key}" "{download}"').execute_test(negative_case=not expect)
    if expect:
        s3api_res.command_is_successful()
    else:
        s3api_res.command_should_fail()


def test_head(**kwargs):
    expect = kwargs.get("expect", True)
    desc = kwargs.get("desc", "Test")
    key = kwargs["key"]
    bucket = kwargs["bucket"]
    s3api_res = AwsTest(desc).with_cli_self(f'aws s3api head-object --output json --bucket "{bucket}" --key "{key}"').execute_test(negative_case=not expect)
    if expect:
        s3api_res.command_is_successful()
        print(f"{json.loads(s3api_res.status.stdout)}")
    else:
        s3api_res.command_should_fail()


def test_del(**kwargs):
    expect = kwargs.get("expect", True)
    desc = kwargs.get("desc", "Test")
    key = kwargs["key"]
    bucket = kwargs["bucket"]
    s3api_res = AwsTest(desc).with_cli_self(f"aws s3api delete-object --bucket {bucket} --key {key}").execute_test(negative_case=not expect)
    if expect:
        s3api_res.command_is_successful()
    else:
        s3api_res.command_should_fail()


def fi_enable(**kwargs):
    desc = kwargs.get("desc", "Enable FI")
    fi = kwargs["fi"]
    freq = kwargs.get("freq", "always")
    if freq == "always":
        S3fiTest(desc).enable_fi("enable", "always", fi).execute_test().command_is_successful()
    else:
        raise Exception(f"FI freq {freq} not supported yet")


def fi_disable(**kwargs):
    desc = kwargs.get("desc", "Disable FI")
    fi = kwargs["fi"]
    S3fiTest(desc).disable_fi(fi).execute_test().command_is_successful()


multipart_map = {}


def test_create_multipart(**kwargs):
    expect = kwargs.get("expect", True)
    desc = kwargs.get("desc", "Test")
    key = kwargs["key"]
    bucket = kwargs["bucket"]
    s3api_res = AwsTest(desc).with_cli_self(f'aws s3api --output json create-multipart-upload --bucket "{bucket}" --key "{key}"').execute_test(negative_case=not expect)
    if expect:
        s3api_res.command_is_successful()
        cmo = json.loads(s3api_res.status.stdout)
        multipart_map[key] = {"UploadId": cmo["UploadId"], "Parts": []}
        print(f"{cmo}")
    else:
        s3api_res.command_should_fail()


def test_upload_part(**kwargs):
    expect = kwargs.get("expect", True)
    desc = kwargs.get("desc", "Test")
    key = kwargs["key"]
    bucket = kwargs["bucket"]
    body = path.abspath(kwargs["body"])
    upload_id = multipart_map[key]["UploadId"]
    part_number = len(multipart_map[key]["Parts"]) + 1
    s3api_res = AwsTest(desc).with_cli_self(f'aws s3api upload-part --output json --bucket "{bucket}" --key "{key}" --part-number "{part_number}" --upload-id "{upload_id}" --body "{body}"').execute_test(negative_case=not expect)
    if expect:
        s3api_res.command_is_successful()
        upo = json.loads(s3api_res.status.stdout)
        p = {"PartNumber": part_number}
        p.update(upo)
        multipart_map[key]["Parts"].append(p)
        print(f"{upo}")
    else:
        s3api_res.command_should_fail()


def test_complete_multipart(**kwargs):
    expect = kwargs.get("expect", True)
    desc = kwargs.get("desc", "Test")
    key = kwargs["key"]
    bucket = kwargs["bucket"]
    upload_id = multipart_map[key]["UploadId"]
    parts = {"Parts": multipart_map[key]["Parts"]}
    parts_json = path.abspath(kwargs["parts_json"])
    with open(parts_json, 'w') as f:
        f.write(json.dumps(parts))

    s3api_res = AwsTest(desc).with_cli_self(f'aws s3api complete-multipart-upload --multipart-upload "file://{parts_json}" --bucket "{bucket}" --key "{key}" --upload-id "{upload_id}"').execute_test(negative_case=not expect)
    if expect:
        s3api_res.command_is_successful()
    else:
        s3api_res.command_should_fail()


def test_list_parts(**kwargs):
    expect = kwargs.get("expect", True)
    desc = kwargs.get("desc", "Test")
    key = kwargs["key"]
    bucket = kwargs["bucket"]
    upload_id = multipart_map[key]["UploadId"]
    s3api_res = AwsTest(desc).with_cli_self(f'aws s3api list-parts --output json --bucket "{bucket}" --key "{key}" --upload-id "{upload_id}"').execute_test(negative_case=not expect)
    if expect:
        s3api_res.command_is_successful()
        print(f"{json.loads(s3api_res.status.stdout)}")
    else:
        s3api_res.command_should_fail()


def test_obj_copy(**kwargs):
    expect = kwargs.get("expect", True)
    desc = kwargs.get("desc", "Test")
    key = kwargs["key"]
    bucket = kwargs["bucket"]
    cpy_src = kwargs["copy-source"]
    s3api_res = AwsTest(desc).with_cli_self(f'aws s3api copy-object --bucket "{bucket}" --key "{key}" --copy-source "{cpy_src}"').execute_test(negative_case=not expect)
    if expect:
        s3api_res.command_is_successful()
    else:
        s3api_res.command_should_fail()


operations_map = {
    "create-bucket": test_create_bucket,
    "delete-bucket": test_delete_bucket,
    "put-object": test_put,
    "get-object":  test_get,
    "head-object": test_head,
    "delete-object": test_del,
    "enable-fi": fi_enable,
    "disable-fi": fi_disable,
    "create-multipart": test_create_multipart,
    "upload-part": test_upload_part,
    "complete-multipart": test_complete_multipart,
    "list-parts": test_list_parts,
    "copy-object": test_obj_copy
}


def process_test_plan(tst_pln, args):
    for t in tst_pln:
        tar = {"body": args.body,
               "bucket": args.bucket,
               "download": args.download,
               "key": args.key,
               "parts_json": args.parts}
        tar.update(t)
        op = tar.get("op", None)
        if op in operations_map:
            operations_map[op](**tar)
        else:
            print(f"Operation {op} is not supported")


def main() -> None:
    random.seed(a=None)
    args, test_plan = get_args()

    test_desc = test_plan.get("desc", "Test")
    print(f"Testing {test_desc}...")

    tests = test_plan.get("tests", [])
    process_test_plan(tests, args)

    print(f"Done {test_desc}")


if __name__ == '__main__':
    main()
