#!/usr/bin/env python3

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

import argparse
from os import path, listdir
from tempfile import TemporaryDirectory
from shutil import unpack_archive, get_unpack_formats
import json
import re
from datetime import datetime

def parse_cmd():
    parser = argparse.ArgumentParser(description='Audit Logs Parser')
    parser.add_argument('-f', '--format', choices=['JSON', 'S3_FORMAT'],
                        required=False, default='JSON',
                        help='S3 Server Audit log format type')
    parser.add_argument('-r', '--recursive', action='store_true', required=False,
                        help='Check sub-directories for log files')
    parser.add_argument('--fields', type=str, nargs="+", required=False, default=[],
                        help='Which fileds should be included into result')
    parser.add_argument('-i', '--indent', type=int, required=False, default=4,
                        help='Indentation to print JSON with')
    parser.add_argument('-c', '--count', action='store_true', required=False,
                        help='Count number of entries')
    parser.add_argument('log_dir', type=str, help='Path to audit logs dir')

    sp = parser.add_subparsers(help='Commands', dest="cmd")

    list_cmd = sp.add_parser("list", help='Show records matching key and value')
    list_cmd.add_argument("-x", action='store_true', help="Records NOT matching [val]", required=False)
    list_cmd.add_argument("key", type=str, help="Specifies record's key")
    list_cmd.add_argument("val", type=str, help="Regexp to match value of the specified key")

    sort_cmd = sp.add_parser("sort", help="Sort records")
    sort_cmd.add_argument("-n", "--num", type=int, help="Print first [NUM] records, could be negative", required=False, default=0)
    sort_cmd.add_argument("-s", "--sortby", type=str, help="Sort by specific field", required=False, default="")
    sort_cmd.add_argument("--dateformat", type=str, help="Format of date field", required=False, default="%d/%b/%Y:%X %z")

    args = parser.parse_args()

    if not path.isdir(args.log_dir):
        raise ValueError('Path to audit logs dir is not accessible')

    return args

exts = [e for sub_exts in map(lambda x: x[1], get_unpack_formats()) for e in sub_exts]
def check_if_arch(fn: str):
    global exts
    bf = path.basename(fn).lower()
    for e in exts:
        if bf.endswith(e):
            return True
    return False

tmp_dir_idx = 0
def list_files(log_dir: str, recursive: bool, tmp: str):
    global tmp_dir_idx
    ret = []
    dir_items = list(map(lambda x: (log_dir, x), listdir(log_dir)))
    for p, di in dir_items:
        full_path = path.join(p, di)
        if path.isfile(full_path):
            if check_if_arch(full_path):
                new_path = path.join(tmp, str(tmp_dir_idx))
                tmp_dir_idx += 1
                unpack_archive(full_path, new_path)
                dir_items += list(map(lambda x: (new_path, x), listdir(new_path)))
            else:
                ret.append(full_path)
        elif path.isdir(full_path) and recursive:
            dir_items += list(map(lambda x: (full_path, x), listdir(full_path)))
    return ret

def rec_from_json(**kwargs):
    audit = dict()
    audit["bucket_owner"] = kwargs.get("bucket_owner", None)
    audit["bucket"] = kwargs.get("bucket", None)
    audit["time"] = kwargs.get("", None)
    audit["remote_ip"] = kwargs.get("remote_ip", None)
    audit["requester"] = kwargs.get("requester", None)
    audit["request_id"] = kwargs.get("request_id", None)
    audit["operation"] = kwargs.get("operation", None)
    audit["key"] = kwargs.get("key", None)
    audit["request_uri"] = kwargs.get("request_uri", None)
    audit["http_status"] = kwargs.get("http_status", None)
    audit["error_code"] = kwargs.get("error_code", None)
    audit["bytes_sent"] = kwargs.get("bytes_sent", None)
    audit["object_size"] = kwargs.get("object_size", None)
    audit["bytes_received"] = kwargs.get("bytes_received", None)
    audit["total_time"] = kwargs.get("total_time", None)
    audit["turn_around_time"] = kwargs.get("turn_around_time", None)
    audit["referrer"] = kwargs.get("referrer", None)
    audit["user_agent"] = kwargs.get("user_agent", None)
    audit["version_id"] = kwargs.get("version_id", None)
    audit["host_id"] = kwargs.get("host_id", None)
    audit["signature_version"] = kwargs.get("signature_version", None)
    audit["cipher_suite"] = kwargs.get("cipher_suite", None)
    audit["authentication_type"] = kwargs.get("authentication_type", None)
    audit["host_header"] = kwargs.get("host_header", None)
    return audit

def rec_from_s3(rec_line: str):
    audit = dict()

    def rec_split(r: str):
        ret_word = ""
        exp_word_delim = " "
        in_word = False
        for c in r:
            if not in_word:
                if c == " ":
                    continue

                if c == "[":
                    exp_word_delim = "]"
                elif c == '"':
                    exp_word_delim = '"'

                ret_word += c
                in_word = True
            else: # in_word is True
                if c == exp_word_delim:
                    ret_word += c
                    yield ret_word.strip('[]" ')
                    ret_word = ""
                    in_word = False
                    exp_word_delim = " "
                else:
                    ret_word += c

    names = ["bucket_owner", "bucket", "time", "remote_ip", "requester", "request_id",
             "operation", "key", "request_uri", "http_status", "error_code", "bytes_sent",
             "object_size", "bytes_received", "total_time", "turn_around_time",
             "referrer", "user_agent", "version_id", "host_id", "signature_version",
             "cipher_suite", "authentication_type", "host_header"]
    for rec_f in rec_split(rec_line):
        if not names:
            break
        audit[names.pop(0)] = rec_f
    return audit

def get_cont(log_file: str, rec_format: str):
    ret = []
    with open(log_file, "r") as lf:
        if rec_format == 'JSON':
            for l in lf:
                if not l:
                    continue
                try:
                    dt = json.loads(l)
                    ret.append(rec_from_json(**dt))
                except json.JSONDecodeError as ex:
                    print(f"Record format error:> {ex}")
        elif rec_format == 'S3_FORMAT':
            for l in lf:
                if not l:
                    continue
                ret.append(rec_from_s3(l))
    return ret

def recs_print(rcs, fields, indent, count):
    i = None if indent == 0 else indent
    ret = []
    if not fields:
        ret = rcs
    else:
        for r in rcs:
            t = {}
            for f in fields:
                if f in r:
                    t[f] = r[f]
            ret.append(t)
    print(json.dumps(ret, indent=i))
    if count:
        print(f"Total records {len(ret)}")

def list_records(ar, k, v, not_matching):
    ret = []
    try:
        tmpl = re.compile(v)
        if not_matching:
            ret = list(filter(lambda rec: k in rec and not tmpl.match(str(rec[k])), ar))
        else:
            ret = list(filter(lambda rec: k in rec and tmpl.match(str(rec[k])), ar))
    except Exception:
        ret = []
    return ret

def sort_records(rcs, num, sortby, dateformat):
    def int_key(r):
        ret = 0
        try:
            ret = int(r[sortby])
        except Exception:
            ret = 0
        return ret

    def str_key(r):
        return r.get(sortby, "")

    def date_key(r):
        ret = datetime.now()
        try:
            ret = datetime.strptime(r[sortby], dateformat)
        except Exception:
            ret = datetime.now()
        return ret

    key_f = None
    if not sortby:
        sortby = "time"
    if sortby == "time":
        key_f = date_key
    elif sortby in ["bytes_sent", "object_size", "bytes_received", "total_time", "turn_around_time"]:
        key_f = int_key
    else:
        key_f = str_key

    rev_flag = num < 0
    num = num if num >= 0 else -num
    rcs.sort(reverse=rev_flag, key=key_f)
    return rcs[:num] if num != 0 else rcs

def main():
    tmp_dir = TemporaryDirectory()
    args = parse_cmd()
    audit_log = []
    for log_file in list_files(args.log_dir, args.recursive, tmp_dir.name):
        tmp = get_cont(log_file, args.format)
        if tmp:
            audit_log += tmp

    filtered_recs = []
    if not args.cmd:
        filtered_recs = audit_log
    elif args.cmd == "list":
        filtered_recs = list_records(audit_log, args.key, args.val, args.x)
    elif args.cmd == "sort":
        filtered_recs = sort_records(audit_log, args.num, args.sortby, args.dateformat)

    recs_print(filtered_recs, args.fields, args.indent, args.count)

    tmp_dir.cleanup()


if __name__ == "__main__":
    main()
