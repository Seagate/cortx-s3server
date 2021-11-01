#!/usr/bin/env python3

#
# Copyright (c) 2021 Seagate Technology LLC and/or its Affiliates
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

# For additional information see docs/object-protection-testing.md

import os
import json
import tempfile

from awss3api import AwsTest


SOURCE_BUCKET = 'replication-source'
DESTINATION_BUCKET = 'replication-dest'
POLICY = {
    "Role": "role-string",
    "Rules": [
        {
            "Status": "Enabled",
            "Prefix": "test",
            "Destination": {
                "Bucket": DESTINATION_BUCKET
            }
        }
    ]
}


def get_replication(should_fail=False):
    test = AwsTest('Get bucket replication').with_cli_self(f"aws s3api --output json get-bucket-replication --bucket {SOURCE_BUCKET}")
    if should_fail:
        test.execute_test(negative_case=True).command_should_fail()
    else:
        return json.loads(test.execute_test().command_is_successful().status.stdout)


def delete_replication():
    AwsTest('Delete bucket replication').with_cli_self(f"aws s3api delete-bucket-replication --bucket {SOURCE_BUCKET}").execute_test().command_is_successful()


def put_replication():
    with tempfile.NamedTemporaryFile(suffix='.json', mode='w') as f:
        json.dump(POLICY, f)
        f.flush()
        os.fsync(f.fileno())
        AwsTest('Put bucket replication').with_cli_self(f"aws s3api put-bucket-replication --bucket {SOURCE_BUCKET} --replication-configuration file://{f.name}").execute_test().command_is_successful()


def is_replicated(key):
    head = json.loads(AwsTest('Head object').with_cli_self(f"aws s3api --output json head-object --bucket {SOURCE_BUCKET} --key {key}").execute_test().command_is_successful().status.stdout)
    return 'ReplicationStatus' in head


if __name__ == '__main__':
    AwsTest('Make replication source bucket').with_cli_self(f"aws s3api create-bucket --bucket {SOURCE_BUCKET}").execute_test().command_is_successful()
    AwsTest('Make replication destination bucket').with_cli_self(f"aws s3api create-bucket --bucket {DESTINATION_BUCKET}").execute_test().command_is_successful()

    get_replication(should_fail=True)
    delete_replication()
    put_replication()
    get_replication()
    delete_replication()
    get_replication(should_fail=True)
    put_replication()
    config = get_replication()
    # Known issue: the old v1 format gets translated to v2,
    # so it doesn't match the provided policy verbatim
    # (though it's functionally equivalent). Try uncommenting
    # the following line once v2 format is available in the
    # test environment.
    #assert(config == {'ReplicationConfiguration': POLICY})

    AwsTest('Put object: wrong prefix').with_cli_self(f"aws s3api put-object --bucket {SOURCE_BUCKET} --key bad1 --body /etc/passwd").execute_test().command_is_successful()
    assert(not is_replicated('bad1'))

    AwsTest('Put object: matching prefix').with_cli_self(f"aws s3api put-object --bucket {SOURCE_BUCKET} --key test1 --body /etc/passwd").execute_test().command_is_successful()
    assert(is_replicated('test1'))
