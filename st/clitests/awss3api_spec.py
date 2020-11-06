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

import os
from framework import Config
from framework import S3PyCliTest
from awss3api import AwsTest
from s3client_config import S3ClientConfig
from aclvalidation import AclTest

# Helps debugging
# Config.log_enabled = True
# Config.dummy_run = True
# Config.client_execution_timeout = 300 * 1000
# Config.request_timeout = 300 * 1000
# Config.socket_timeout = 300 * 1000

# Transform AWS CLI text output into an object(dictionary)
# with content:
# {
#   "prefix":<list of common prefix>,
#   "keys": <list of regular keys>,
#   "next_token": <token>
# }
def get_aws_cli_object(raw_aws_cli_output):
    cli_obj = {}
    raw_lines = raw_aws_cli_output.split('\n')
    common_prefixes = []
    content_keys = []
    for _, item in enumerate(raw_lines):
        if (item.startswith("COMMONPREFIXES")):
            # E.g. COMMONPREFIXES  quax/
            line = item.split('\t')
            common_prefixes.append(line[1])
        elif (item.startswith("CONTENTS")):
            # E.g. CONTENTS\t"98b5e3f766f63787ea1ddc35319cedf7"\tasdf\t2020-09-25T11:42:54.000Z\t3072\tSTANDARD
            line = item.split('\t')
            content_keys.append(line[2])
        elif (item.startswith("NEXTTOKEN")):
            # E.g. NEXTTOKEN       eyJDb250aW51YXRpb25Ub2tlbiI6IG51bGwsICJib3RvX3RydW5jYXRlX2Ftb3VudCI6IDN9
            line = item.split('\t')
            cli_obj["next_token"] = line[1]
        else:
            continue

    if (common_prefixes is not None):
        cli_obj["prefix"] = common_prefixes
    if (content_keys is not None):
        cli_obj["keys"] = content_keys

    return cli_obj

# Extract the upload id from response which has the following format
# [bucketname    objecctname    uploadid]

def get_upload_id(response):
    key_pairs = response.split('\t')
    return key_pairs[2]

# Run before all to setup the test environment.
print("Configuring LDAP")
S3PyCliTest('Before_all').before_all()

def create_object_list_file(file_name, obj_list=[], quiet_mode="false"):
    cwd = os.getcwd()
    file_to_create = os.path.join(cwd, file_name)
    objects = "{ \"Objects\": [ { \"Key\": \"" + obj_list[0] + "\" }"
    for obj in obj_list[1:]:
        objects += ", { \"Key\": \"" + obj + "\" }"
    objects += " ], \"Quiet\": " + quiet_mode + " }"
    with open(file_to_create, 'w') as file:
        file.write(objects)
    return file_to_create

def delete_object_list_file(file_name):
    os.remove(file_name)

#******** Create Bucket ********
AwsTest('Aws can create bucket').create_bucket("seagatebucket").execute_test().command_is_successful()

#******** Verify pagination and NextToken in List objects V1  **********
# Step 1. Create total 110 objets in bucket
obj_110_list = []
for x in range(110):
    key = "object%d" % (x)
    AwsTest(('Aws Upload object: %s' % key)).put_object("seagatebucket", "3Kfile", 3000, key_name=key)\
        .execute_test().command_is_successful()
    obj_110_list.append(key)

# Step 2.
# Create object list file
object_list_file = create_object_list_file("obj_list_110_keys.json", obj_110_list, "true")

# Step 3. List first 100 objects. This will result into NextToken != None
# NextToken will point to object9 - the last object key in the response.
result = AwsTest('Aws list the first 100 objects')\
    .list_objects("seagatebucket", None, "100")\
    .execute_test()\
    .command_is_successful()

# Step 4. Validate
list_response = get_aws_cli_object(result.status.stdout)
print("List response: %s\n", str(list_response))
# Verify 'object9' is present and 'object99' is not present in the resultant list 'list_response'
if list_response is not None:
    assert 'object9' in list_response["keys"], "Failed to see object9 in the response"
    assert 'object99' not in list_response["keys"], "Failed: Unexpected key object99 in the response"
else:
    assert False, "Failed to list objects from bucket"
# Verify that NextToken is in 'list_response' and it is not empty
assert (("next_token" in list_response.keys()) and (len(list_response["next_token"].strip()) > 0)), "NextToken is either not present or empty"

# Step 5. Cleanup: Delete 110 objects, and remove object list file containing a list of 110 objects
# Delete all 110 objects in bucket
AwsTest('Aws delete objects: [object0...object109]')\
    .delete_multiple_objects("seagatebucket", object_list_file)\
    .execute_test()\
    .command_is_successful()\
    .command_response_should_be_empty()

delete_object_list_file(object_list_file)


# ******** Create hierarchical objects (>30) in bucket and list them *********
obj_list = []
for x in range(35):
    key = "test/test1/key%d" % (x)
    AwsTest('Aws Upload object with hierarchical key path').put_object("seagatebucket", "3Kfile", 3000, key_name=key)\
        .execute_test().command_is_successful()
    obj_list.append(key)

# Create object list file
hierarchical_object_list_file = create_object_list_file("obj_list_hierarchical_keys.json", obj_list, "true")
# Lists hierarchical objects in bucket with delimiter "/"
# The output of list objects should only have common prefix: test/
AwsTest('Aws List objects with hierarchical key paths, using delimiter "/"')\
    .list_objects_prefix_delimiter("seagatebucket", None, None, "/")\
    .execute_test().command_is_successful().command_response_should_have("test/")

# Delete all hierarchical objects in bucket
AwsTest('Aws delete multiple objects with hierarchical names in bucket')\
    .delete_multiple_objects("seagatebucket", hierarchical_object_list_file)\
    .execute_test()\
    .command_is_successful()\
    .command_response_should_be_empty()

# Delete object list file
delete_object_list_file(hierarchical_object_list_file)


# ************ Put object with specified content-type ************
in_headers = {
    "content-type" : "application/blabla"
    }
AwsTest('Aws can upload object with specific content-type').put_object("seagatebucket", "3kfile", 3000)\
    .add_headers(in_headers).execute_test().command_is_successful()

AwsTest('Aws can get object with the same content-type').get_object("seagatebucket", "3kfile")\
    .execute_test().command_is_successful().command_response_should_have("application/blabla")

AwsTest('Aws can delete object').delete_object("seagatebucket", "3kfile")\
    .execute_test().command_is_successful()

#*********** Verify aws s3api list-objects-v2 ***********
# Setup objects in bucket
obj_list = ['asdf', 'quax/t3/key.log', 'foo', 'quax/t1/key.log', 'boo', 'zoo/p2/key.log', 'zoo/p7/key.log']
# Create above objects in existing bucket 'seagatebucket'
for x in obj_list:
    AwsTest(('Aws upload object:%s' % x)).put_object("seagatebucket", "1kfile", 1000, key_name=x)\
        .execute_test().command_is_successful()

# Start Listing objects using V2 scheme
# Test #1: List objects using --start-after, --max-items and --starting-token
list_v2_options = {"start-after":"boo", "max-items":2}
result = AwsTest('Aws list-objects-v2: start-after and max-items').list_objects_v2("seagatebucket", **list_v2_options)\
    .execute_test().command_is_successful().command_response_should_have("quax/t1/key.log")\
    .command_response_should_have("foo")

# Process result set
lv2_response = get_aws_cli_object(result.status.stdout)
# Get next-token from lv2_response
if ("next_token" in lv2_response.keys()):
    Next_token = lv2_response["next_token"]

# Get keys from lv2_response
obj_keys =  lv2_response["keys"]

# Assert for key existence in response
assert "foo" in obj_keys, "Key \"foo\" is expected to be in the result"
assert "quax/t1/key.log" in obj_keys, "Key \"quax/t1/key.log\" is expected to be in the result"
# Assert for key non-existence in response
# key 'boo' and other keys (from key 'quax/t3/key.log' onwards) should not be in the response
assert "boo" not in obj_keys, "Key \"boo\" should not be in the result"
# 'quax/t3/key.log' shouldn't be in response, as --max-items=2
assert "quax/t3/key.log" not in obj_keys, "Key \"quax/t3/key.log\" should not be in the result due to pagination"

assert Next_token is not None

# Further list objects using --starting-token
list_v2_options = {"starting-token":Next_token}
result = AwsTest('Aws list-objects-v2: starting-token').list_objects_v2("seagatebucket", **list_v2_options)\
    .execute_test().command_is_successful().command_response_should_not_have("NextToken")
# Process result set
lv2_response = get_aws_cli_object(result.status.stdout)
# next_token should not be in 'lv2_response' at this point
if ("next_token" in lv2_response.keys()):
    Next_token = lv2_response["next_token"]
else:
    Next_token = None
# Get keys from lv2_response
obj_keys =  lv2_response["keys"]
# next-token should be None
assert (Next_token is None)
assert len(obj_keys) == 5, ("Expected 5, Actual = %d") & (len(obj_keys))

# Test #2: List objects using --delimiter
# List objects using delimiter="/"
list_v2_options = {"delimiter":"/"}
result = AwsTest('Aws list objects using V2 scheme').list_objects_v2("seagatebucket", **list_v2_options)\
    .execute_test().command_is_successful()
# Verify contents of the test output
lv2_response = get_aws_cli_object(result.status.stdout)
assert len(lv2_response["prefix"]) == 2, (("Expecting common prefixes:{quax/, zoo/}\n. Actual:%s") % str((lv2_response["prefix"])))
assert len(lv2_response["keys"]) == 3, (("Expecting keys:{asdf, boo, foo} in the output. Actual:%s") % str((lv2_response["keys"])))

# Delete all objects created for list-objects-v2 test
for x in obj_list:
    AwsTest(('Aws delete object:%s' % x)).delete_object("seagatebucket", x)\
        .execute_test().command_is_successful()


# ********* Specific ListObject V2 hierarchical object key tests  **********
# Create below hierarchical objects in bucket and list them in different ways
# test/.uds/volumeGroupA/vol0...vol30
# test/.uds/volumeGroupB/lun0...lun4
# test/.uds/.metadata0 ... .metadata2
# test/.uds/.backup/vol_backup/files0...files19
# test/.uds/.backup/storage/diskgroup/.backup/2020-09/data/.files
# .a1, .f1, .p1
obj_list = []
for x in range(31):
    key = "test/.uds/volumeGroupA/vol%d" % (x)
    AwsTest('Aws Upload object:%s' % key).put_object("seagatebucket", "1Kfile", 1000, key_name=key)\
        .execute_test().command_is_successful()
    obj_list.append(key)

for x in range(5):
    key = "test/.uds/volumeGroupB/lun%d" % (x)
    AwsTest('Aws Upload object:%s' % key).put_object("seagatebucket", "1Kfile", 1000, key_name=key)\
        .execute_test().command_is_successful()
    obj_list.append(key)

for x in range(3):
    key = "test/.uds/.metadata%d" % (x)
    AwsTest('Aws Upload object:%s' % key).put_object("seagatebucket", "1Kfile", 1000, key_name=key)\
        .execute_test().command_is_successful()
    obj_list.append(key)

for x in range(20):
    key = "test/.uds/.backup/vol_backup/files%d" % (x)
    AwsTest('Aws Upload object:%s' % key).put_object("seagatebucket", "1Kfile", 1000, key_name=key)\
        .execute_test().command_is_successful()
    obj_list.append(key)
# Long path
key = "test/.uds/.backup/storage/diskgroup/.backup/2020-09/data/.files"
AwsTest('Aws Upload object:%s' % key).put_object("seagatebucket", "1Kfile", 1000, key_name=key)\
    .execute_test().command_is_successful()
obj_list.append(key)

keys = [".a1", ".f1", ".p1"]
for key in keys:
    AwsTest('Aws Upload object:%s' % key).put_object("seagatebucket", "1Kfile", 1000, key_name=key)\
        .execute_test().command_is_successful()
    obj_list.append(key)

# Listing#1: aws s3api list-objects --bucket cortx --delimiter "/"
# Expected output:
# Common prefix should have ["test/"]
# Contents/keys should have [".a1", ".f1", ".p1"]
result = AwsTest('Aws List objects with hierarchical key paths, using delimiter:/')\
    .list_objects_prefix_delimiter("seagatebucket", None, None, "/")\
    .execute_test().command_is_successful()

lv2_response = get_aws_cli_object(result.status.stdout)
assert (len(lv2_response["prefix"]) == 1)
common_prefix = lv2_response["prefix"]
assert ("test/" in common_prefix), "Expected: \"test/\", Actual:%s" % str(common_prefix)
assert len(lv2_response["keys"]) == 3, (("Expected:[.a1, .f1, .p1]. Actual:%s") % str((lv2_response["keys"])))
assert all(item in lv2_response["keys"] for item in keys)

# Listing#2: aws s3api list-objects --bucket cortx --prefix "test/.uds/" --delimiter "/"
# Expected output:
# Common prefix should have: [test/.uds/.backup/, test/.uds/volumeGroupA/, test/.uds/volumeGroupB/]
# Contents/keys should have [test/.uds/.metadata1, test/.uds/.metadata2, test/.uds/.metadata3]
result = AwsTest('Aws List objects with hierarchical key paths, using prefix:%s delimiter:/' % "test/.uds/")\
    .list_objects_prefix_delimiter("seagatebucket", None, "test/.uds/", "/")\
    .execute_test().command_is_successful()

lv2_response = get_aws_cli_object(result.status.stdout)
assert (len(lv2_response["prefix"]) == 3)
common_prefix = ['test/.uds/.backup/', 'test/.uds/volumeGroupA/', 'test/.uds/volumeGroupB/']
keys = ["test/.uds/.metadata0", "test/.uds/.metadata1", "test/.uds/.metadata2"]
assert all(item in lv2_response["prefix"] for item in common_prefix)
assert (len(lv2_response["keys"]) == 3)
assert all(item in lv2_response["keys"] for item in keys)

# Listing#3: aws s3api list-objects --bucket cortx --prefix "test/.uds/.backup"
# Expected output:
# Common prefix should be: []
# Contents/keys should have [test/.uds/.backup/storage/diskgroup/.backup/2020-09/data/files, test/.uds/.backup/vol_backup/files{1..20}]
result = AwsTest('Aws List objects with hierarchical key paths, using prefix:%s' % "test/.uds/.backup")\
    .list_objects_prefix_delimiter("seagatebucket", None, "test/.uds/.backup", None)\
    .execute_test().command_is_successful()

lv2_response = get_aws_cli_object(result.status.stdout)
common_prefix = lv2_response.get("prefix")
if common_prefix is not None:
    assert len(common_prefix) == 0

assert (len(lv2_response["keys"]) == 21), "Expect 21 regular keys in the result, Actual = %d" % len(lv2_response["keys"])

# Listing#4: aws s3api list-objects --bucket cortx --prefix "test/.uds/.backup/" --delimiter "/"
# Expected output:
# Common prefix should be: [test/.uds/.backup/storage/, test/.uds/.backup/vol_backup/]
# Contents/keys should be []
result = AwsTest('Aws List objects with hierarchical key paths, using prefix:%s, delimiter:/' % "test/.uds/.backup/")\
    .list_objects_prefix_delimiter("seagatebucket", None, "test/.uds/.backup/", "/")\
    .execute_test().command_is_successful()

lv2_response = get_aws_cli_object(result.status.stdout)
common_prefix = ["test/.uds/.backup/storage/", "test/.uds/.backup/vol_backup/"]
assert (len(lv2_response["prefix"]) == 2)
assert all(item in lv2_response["prefix"] for item in common_prefix)
keys = lv2_response.get("keys")
if keys is not None:
    assert len(keys) == 0


# Listing#4.1: aws s3api list-objects --bucket cortx --prefix "test/.uds/.backup" --delimiter "/"
# Expected output:
# Common prefix should be: [test/.uds/.backup/]
# Contents/keys should be []
result = AwsTest('Aws List objects with hierarchical key paths, using prefix:%s, delimiter:/' % "test/.uds/.backup")\
    .list_objects_prefix_delimiter("seagatebucket", None, "test/.uds/.backup", "/")\
    .execute_test().command_is_successful()

lv2_response = get_aws_cli_object(result.status.stdout)
common_prefix = ["test/.uds/.backup/"]
assert (len(lv2_response["prefix"]) == 1)
assert (common_prefix == lv2_response["prefix"])
keys = lv2_response.get("keys")
if keys is not None:
    assert len(keys) == 0


# Listing#5: aws s3api list-objects --bucket cortx --prefix "test/.uds/backup"
# Expected output:
# No keys or common prefix in result, as there i no key that starts with "test/.uds/backup"
# Common prefix should be: []
# Contents/keys should be []
result = AwsTest('Aws List objects with hierarchical key paths, using prefix:%s' % "test/.uds/backup")\
    .list_objects_prefix_delimiter("seagatebucket", None, "test/.uds/backup", None)\
    .execute_test().command_is_successful()

lv2_response = get_aws_cli_object(result.status.stdout)
assert len(lv2_response["prefix"]) == 0
assert len(lv2_response["keys"]) == 0

# Delete all hierarchical objects created for list object hierarchical test
for x in obj_list:
    AwsTest(('Aws delete object:%s' % x)).delete_object("seagatebucket", x)\
        .execute_test().command_is_successful()



#******** Put Bucket Tag ********
AwsTest('Aws can create bucket tag').put_bucket_tagging("seagatebucket", [{'Key': 'organization','Value': 'marketing'}])\
    .execute_test().command_is_successful()

#******** List Bucket Tag ********
AwsTest('Aws can list bucket tag').list_bucket_tagging("seagatebucket").execute_test().command_is_successful()\
    .command_response_should_have("organization").command_response_should_have("marketing")

#******** List bucket Tag after Updating tags ********
AwsTest('Aws can create bucket tag').put_bucket_tagging("seagatebucket", [{'Key': 'seagate','Value': 'storage'}])\
    .execute_test().command_is_successful()

AwsTest('Aws can list bucket tag').list_bucket_tagging("seagatebucket").execute_test().command_is_successful()\
    .command_response_should_not_have("organization").command_response_should_not_have("marketing")\
    .command_response_should_have("seagate").command_response_should_have("storage")

#********** Negative case to check restriction on length of bucket-tagging key length ***********
invalid_long_tag_key = 'LiXrj2I0fMJNAyhl61P7a536zhZXgkAZCNfaTLy11SkNcxmMPd7wrVf6hpHYgmw0r5KiNgt5vSFcLfmgoiQ5hizpCLotjoIWj9TYdO02NTg58XGMUX8Xol2wez42UnZb1'

AwsTest('Aws can not create bucket tag with key size greater than 128 unicode characters')\
    .put_bucket_tagging("seagatebucket", [{'Key': invalid_long_tag_key,'Value': 'marketing'}])\
    .execute_test(negative_case=True).command_should_fail().command_error_should_have("InvalidTagError")

#*********** Negative case to check restriction on length of bucket-tagging value length *********
invalid_long_tag_value = 'kJHu017I0OErlyoZLfx6fKodbEWXBBJjvCt0YuliSZvfBNPlgvecAZkUjwtG3YHRMpTDRcMMcsQAfTlrq47ZRvzsVLBAkFgBLFm9ytbfV2E6Vo6NX4uEnMaVFq9z7Mh9XzRZaCdrF7plwiMgjs1hQbOOm2VLXiVf31paJLipWw2D66DExvp0e1IqufzGQ5KzvEu5vJRAFtfiXLRhsRHWV0cvli0wVSWpK6Wg3NbSz7kMrX2MqIQ5gyZNWeT4ISzJNjLS7BVljDPh4IOSn6KAjEQLFeOZzmvUwcSU8VG92mL14EEZqzu8etBYzK499BEfM67B9bkAmiwu3Lpr6DZk6a0vBwLfBijVyOfDhGTzkXxYnqjYnGZ65S4sfnS4yeBeI0KGWAIVa39I3VwAwigL3lAmgqxrYSVJN0JgHVSapE9lb80YjVoADE6jtLAeXbv3YGfbUtagpSaj16kVdueW1wGlDmNLTuZOhKW3cSOnXLhhVKbLCff4l8CA9lh70q1Vs'

AwsTest('Aws can not create bucket tag with value size greater than 2*256 unicode characters')\
    .put_bucket_tagging("seagatebucket", [{'Key': 'organization','Value': invalid_long_tag_value}])\
    .execute_test(negative_case=True).command_should_fail().command_error_should_have("InvalidTagError")

#*********** Negative case to check restriction on length of bucket-tagging value length *********
tag_list_with_too_many_tags = []
for x in range(52):
    key = "seagate" + str(x)
    value = "storage" + str(x)
    case = {'Key': key, 'Value': value}
    tag_list_with_too_many_tags.append(case)

AwsTest('Aws can create bucket tag for more than 50 entries').put_bucket_tagging("seagatebucket", tag_list_with_too_many_tags).execute_test(negative_case=True)\
    .command_should_fail().command_error_should_have("InvalidTagError")


#*********** Negative case to check invalid char(s) for key in bucket-tagging *********
invalid_chars = ["!", "^", "%", "<", ">", "[", "]", "~", "|", "#", "?", "@", "*"]
for char in invalid_chars:
    invalid_key = "sea"+ char + "gate"
    AwsTest('Aws can not create bucket tag with invalid char(' + char +') in Key').put_bucket_tagging("seagatebucket",  [{'Key': invalid_key, 'Value': 'storage'}]).execute_test(negative_case=True)\
    .command_should_fail().command_error_should_have("InvalidTagError")

#*********** Negative case to check invalid char(s) for value in bucket-tagging *********
#invalid_chars = ["!", "^", "%", "<", ">", "[", "]", "~", "|", "#", "?", "@", "*"]
for char in invalid_chars:
    invalid_value = "stor"+ char + "age"
    AwsTest('Aws can not create bucket tag with invalid char(' + char +') in Value').put_bucket_tagging("seagatebucket",  [{'Key': 'seagate', 'Value': invalid_value}]).execute_test(negative_case=True)\
    .command_should_fail().command_error_should_have("InvalidTagError")

#************Negative case to check creation of tag for non-existant bucket*******
AwsTest('Aws can not create bucket tag on non-existant').put_bucket_tagging("seagate-bucket", [{'Key': 'seagate','Value': 'storage'}])\
    .execute_test(negative_case=True).command_should_fail().command_error_should_have("NoSuchBucket")

#************** Delete Bucket Tag ********
AwsTest('Aws can delete bucket tag').delete_bucket_tagging("seagatebucket").execute_test().command_is_successful()

AwsTest('Aws can list bucket tag').list_bucket_tagging("seagatebucket").execute_test(negative_case=True)\
    .command_should_fail().command_error_should_have("NoSuchTagSetError")

#******* delete bucket **********
AwsTest('Aws can delete bucket').delete_bucket("seagatebucket").execute_test().command_is_successful()

#******** Create Bucket ********
AwsTest('Aws can create bucket').create_bucket("seagatebuckettag").execute_test().command_is_successful()

#******** Put Object Tag ********
AwsTest('Aws can upload 3k file with tags').put_object_with_tagging("seagatebuckettag", "3kfile", 3000, "organization=marketing" ).execute_test().command_is_successful()

#******** List Object Tag ********
AwsTest('Aws can list object tag').list_object_tagging("seagatebuckettag","3kfile").execute_test().command_is_successful()\
    .command_response_should_have("organization").command_response_should_have("marketing")

#******** List Object Tag after Updating tags ********
AwsTest('Aws can update tags for 3k file').put_object_tagging("seagatebuckettag", "3kfile", [{'Key': 'seagate','Value': 'storage'}] ).execute_test().command_is_successful()

AwsTest('Aws can list bucket tag after update').list_object_tagging("seagatebuckettag","3kfile").execute_test().command_is_successful()\
    .command_response_should_not_have("organization").command_response_should_not_have("marketing")\
    .command_response_should_have("seagate").command_response_should_have("storage")

#*********** Negative case to check restriction on length of object-tagging value length *********
tag_list_with_too_many_tags = []
for x in range(11):
    key = "seagate" + str(x)
    value = "storage" + str(x)
    case = {'Key': key, 'Value': value}
    tag_list_with_too_many_tags.append(case)

AwsTest('Aws can create object tag for more than 10 entries').put_object_tagging("seagatebuckettag","3kfile", tag_list_with_too_many_tags).execute_test(negative_case=True)\
    .command_should_fail().command_error_should_have("InvalidTagError")

#************** Delete Object Tag ********
AwsTest('Aws can delete object tag').delete_object_tagging("seagatebuckettag","3kfile").execute_test().command_is_successful()

AwsTest('Aws can list object tag').list_object_tagging("seagatebuckettag","3kfile").execute_test()\
    .command_is_successful().command_response_should_not_have("NoSuchTagSetError")

#******** Object tags with UTF-8 ********
AwsTest('Aws can create object tag with 2-bytes UTF-8 characters').put_object_tagging("seagatebuckettag", "3kfile",  [{'Key': 'utf-8_1', 'Value': '\u0410\u0411\u0412\u0413' }])\
    .execute_test().command_is_successful()

AwsTest('Aws can create object tag with 3-bytes UTF-8 characters').put_object_tagging("seagatebuckettag", "3kfile",  [{'Key': 'utf-8_2', 'Value': '\u20AC\u20AD'}])\
    .execute_test().command_is_successful()

# In the next line we will create 256-length string of 4-byte UTF-8 symbols
long_utf8_4bytes_string = b'\xF0\x90\x8D\x88'.decode('utf-8') * 256
AwsTest('Aws can create object tag with long value of 4-bytes UTF-8 characters').put_object_tagging("seagatebuckettag", "3kfile",  [{'Key': 'utf-8_3', 'Value': long_utf8_4bytes_string}])\
    .execute_test().command_is_successful()

long_utf8_4bytes_string = b'\xF0\x90\x8D\x88'.decode('utf-8') * 128
AwsTest('Aws can create object tag with long name of 4-bytes UTF-8 characters').put_object_tagging("seagatebuckettag", "3kfile",  [{'Key': long_utf8_4bytes_string, 'Value': 'utf-8_3'}])\
    .execute_test().command_is_successful()

#*********** Negative case to check invalid char(s) for key in object-tagging *********
invalid_chars = ["!", "^", "%", "<", ">", "[", "]", "~", "|", "#", "?", "*"]
for char in invalid_chars:
    invalid_key = "sea"+ char + "gate"
    AwsTest('Aws can not create object tag with invalid char(' + char +') in Key').put_object_tagging("seagatebuckettag", "3kfile",  [{'Key': invalid_key, 'Value': 'storage'}])\
    .execute_test(negative_case=True).command_should_fail().command_error_should_have("InvalidTagError")

#*********** Negative case to check invalid char(s) for value in object-tagging *********
for char in invalid_chars:
    invalid_value = "stor"+ char + "age"
    AwsTest('Aws can not create object tag with invalid char(' + char +') in Value').put_object_tagging("seagatebuckettag", "3kfile",  [{'Key': 'seagate', 'Value': invalid_value}])\
    .execute_test(negative_case=True).command_should_fail().command_error_should_have("InvalidTagError")

#************** Delete Object  ********
AwsTest('Aws can delete object').delete_object("seagatebuckettag","3kfile").execute_test().command_is_successful()

#************** Create a multipart upload ********
result=AwsTest('Aws can upload 10Mb file with tags').create_multipart_upload("seagatebuckettag", "10Mbfile", 10485760, "domain=storage" ).execute_test().command_is_successful()
upload_id = get_upload_id(result.status.stdout)
print(upload_id)

#************** Upload Individual parts ********
result=AwsTest('Aws can upload 5Mb first part').upload_part("seagatebuckettag", "firstpart", 5242880, "10Mbfile", "1" , upload_id).execute_test().command_is_successful()
e_tag_1 = result.status.stdout
print(e_tag_1)

result=AwsTest('Aws can upload 5Mb second part').upload_part("seagatebuckettag", "secondpart", 5242880, "10Mbfile", "2" , upload_id).execute_test().command_is_successful()
e_tag_2 = result.status.stdout
print(e_tag_2)

parts="Parts=[{ETag="+e_tag_1.strip('\n')+",PartNumber="+str(1)+"},{ETag="+e_tag_2.strip('\n')+",PartNumber="+str(2)+"}]"
print(parts)

#************** Get object acl should fail before complete multipart upload ******
AwsTest('Aws can get object acl').get_object_acl("seagatebuckettag", "10Mbfile").execute_test(negative_case=True)\
.command_should_fail().command_error_should_have("NoSuchKey")

#************** Complete multipart upload ********
result=AwsTest('Aws can complete multipart upload 10Mb file with tags').complete_multipart_upload("seagatebuckettag", "10Mbfile", parts, upload_id).execute_test().command_is_successful().command_response_should_have("seagatebuckettag/10Mbfile")

#************* Get object ACL for multipart Upload **********

aclresult=AwsTest('Aws can get object acl').get_object_acl("seagatebuckettag", "10Mbfile").execute_test().command_is_successful()

print("ACL validation started..")
AclTest('validate complete acl').validate_acl(aclresult, "C12345", "s3_test", "C12345", "s3_test", "FULL_CONTROL")
AclTest('acl has valid Owner').validate_owner(aclresult, "C12345", "s3_test")
AclTest('acl has valid Grants').validate_grant(aclresult, "C12345", "s3_test", 1, "FULL_CONTROL")
print("ACL validation Completed..")

#******** List Object Tags for Multipart Upload********
AwsTest('Aws can list object tags for multipart upload').list_object_tagging("seagatebuckettag","10Mbfile").execute_test().command_is_successful()\
    .command_response_should_have("domain").command_response_should_have("storage")

#************** Delete Object  ********
AwsTest('Aws can delete object').delete_object("seagatebuckettag","10Mbfile").execute_test().command_is_successful()

# ************** Delete multiple objects, all objects exist, Quiet mode = True *****************
# 1. Create 3 object keys in bucket
# 2. Delete all 3 objects
# 3. Check If all got deleted and we have empty XML response

# *************** create 3 object keys in the bucket *******************************************
AwsTest('Aws can upload 1kfileA').put_object("seagatebuckettag", "1kfileA", 1000).execute_test().command_is_successful()
AwsTest('Aws can upload 1kfileB').put_object("seagatebuckettag", "1kfileB", 1000).execute_test().command_is_successful()
AwsTest('Aws can upload 1kfileC').put_object("seagatebuckettag", "1kfileC", 1000).execute_test().command_is_successful()

# **************** Create file with the above created objects **********************************
object_list_file = create_object_list_file("obj_list_file_all_exist.json", ["1kfileA", "1kfileB", "1kfileC"], "true")

# **************** Delete multiple objects *****************************************************
AwsTest('Aws can delete multiple objects when all objects exist in bucket, quiet_mode: true')\
    .delete_multiple_objects("seagatebuckettag", object_list_file)\
    .execute_test()\
    .command_is_successful()\
    .command_response_should_be_empty()

# ***************** validate list objects do not have above objects ****************************
AwsTest('Aws do not list objects after multiple delete')\
    .list_objects("seagatebuckettag")\
    .execute_test()\
    .command_is_successful()\
    .command_response_should_not_have('1kfile')

# **************** Delete the object list file *************************************************
delete_object_list_file(object_list_file)

###########################################################

# ************** Delete multiple objects, some objects do not exist, Quiet mode = True *********
# 1. Create 2 object keys in bucket
# 2. Delete 3 objects, with 1 object being non-existent
# 3. Check if we got empty XML response.

# *************** create 2 object keys in the bucket *******************************************
AwsTest('Aws can upload 1kfileA').put_object("seagatebuckettag", "1kfileD", 1000).execute_test().command_is_successful()
AwsTest('Aws can upload 1kfileB').put_object("seagatebuckettag", "1kfileE", 1000).execute_test().command_is_successful()

# **************** Create file with the above created objects and one fake object **************
object_list_file = create_object_list_file("obj_list_file_not_all_exist.json", ["1kfileD", "1kfileE", "fakefileA"], "true")

# **************** Delete multiple objects *****************************************************
AwsTest('Aws can delete multiple objects when some objects do not exist in bucket, quiet_mode: true')\
    .delete_multiple_objects("seagatebuckettag", object_list_file)\
    .execute_test()\
    .command_is_successful()\
    .command_response_should_be_empty()

# ***************** validate list objects do not have above objects ****************************
AwsTest('Aws do not list objects after multiple delete')\
    .list_objects("seagatebuckettag")\
    .execute_test()\
    .command_is_successful()\
    .command_response_should_not_have('1kfile')


# **************** Delete the object list file *************************************************
delete_object_list_file(object_list_file)

###########################################################################

# ************** Delete multiple objects, all objects do not exist, Quiet mode = True **********
# 1. Delete 3 non-existent objects
# 2. Verify empty XML response

# **************** Create file with the all fake objects **************
object_list_file = create_object_list_file("obj_list_file_all_not_exist.json", ["fakefileB", "fakefileC", "fakefileD"], "true")

# **************** Delete multiple objects *****************************************************
AwsTest('Aws can delete multiple objects when no object exist in bucket, quiet_mode: true')\
    .delete_multiple_objects("seagatebuckettag", object_list_file)\
    .execute_test()\
    .command_is_successful()\
    .command_response_should_be_empty()

# **************** Delete the object list file *************************************************
delete_object_list_file(object_list_file)

##############################################################################

# **************** Delete multiple objects, all objects exist, Quiet mode = False
# 1. Create 3 object keys in bucket
# 2. Delete all 3 objects with quiet_mode="false"
# 3. Check If all got deleted and we do not have empty XML response

# *************** create 3 object keys in the bucket *******************************************
AwsTest('Aws can upload 1kfileF').put_object("seagatebuckettag", "1kfileF", 1000).execute_test().command_is_successful()
AwsTest('Aws can upload 1kfileG').put_object("seagatebuckettag", "1kfileG", 1000).execute_test().command_is_successful()
AwsTest('Aws can upload 1kfileH').put_object("seagatebuckettag", "1kfileH", 1000).execute_test().command_is_successful()

# **************** Create file with the above created objects **********************************
object_list_file = create_object_list_file("obj_list_file_all_exist_not_quiet_mode.json", ["1kfileF", "1kfileG", "1kfileH"], "false")

# **************** Delete multiple objects *****************************************************
AwsTest('Aws can delete multiple objects when all objects exist in bucket, quiet_mode: false')\
    .delete_multiple_objects("seagatebuckettag", object_list_file)\
    .execute_test()\
    .command_is_successful()\
    .command_response_should_have('1kfileF')\
    .command_response_should_have('1kfileG')\
    .command_response_should_have('1kfileH')

# ***************** validate list objects do not have above objects ****************************
AwsTest('Aws do not list objects after multiple delete')\
    .list_objects("seagatebuckettag")\
    .execute_test()\
    .command_is_successful()\
    .command_response_should_not_have('1kfile')

# **************** Delete the object list file *************************************************
delete_object_list_file(object_list_file)

################################################################################

#******* Multipart upload should fail if non-last part is less then 5 Mb ******

result=AwsTest('Multipart upload with wrong-sized parts').create_multipart_upload("seagatebuckettag", "10Mbfile", 10485760, "domain=storage" ).execute_test().command_is_successful()
upload_id = get_upload_id(result.status.stdout)
print(upload_id)

result=AwsTest('Aws can upload 4Mb first part').upload_part("seagatebuckettag", "firstpart", 4194304, "10Mbfile", "1" , upload_id).execute_test().command_is_successful()
e_tag_1 = result.status.stdout
print(e_tag_1)

result=AwsTest('Aws can upload 4Mb second part').upload_part("seagatebuckettag", "secondpart", 4194304, "10Mbfile", "2" , upload_id).execute_test().command_is_successful()
e_tag_2 = result.status.stdout
print(e_tag_2)

result=AwsTest('Aws can upload 2Mb third part').upload_part("seagatebuckettag", "thirdpart", 2097152, "10Mbfile", "3" , upload_id).execute_test().command_is_successful()
e_tag_3 = result.status.stdout
print(e_tag_3)

parts="Parts=[{ETag="+e_tag_1.strip('\n')+",PartNumber=1},{ETag="+e_tag_2.strip('\n')+",PartNumber=2},{ETag="+e_tag_3.strip('\n')+",PartNumber=3}]"
print(parts)

result=AwsTest('Aws cannot complete multipart upload when non-last part is less than 5MB').complete_multipart_upload("seagatebuckettag", "10Mbfile", parts, upload_id).execute_test(negative_case=True).command_should_fail().command_error_should_have("EntityTooSmall")

#******* Multipart upload should fail if user completes multipart-upload with wrong ETag ******

result=AwsTest('Multipart upload with wrong ETag').create_multipart_upload("seagatebuckettag", "10Mbfile", 10485760, "domain=storage" ).execute_test().command_is_successful()
upload_id = get_upload_id(result.status.stdout)
print(upload_id)

parts="Parts=[{ETag=00000000000000000000000000000000,PartNumber=1}]"

result=AwsTest('Aws cannot complete multipart upload with wrong ETag').complete_multipart_upload("seagatebuckettag", "10Mbfile", parts, upload_id).execute_test(negative_case=True).command_should_fail().command_error_should_have("InvalidPart")

#******* Delete bucket **********
AwsTest('Aws can delete bucket').delete_bucket("seagatebuckettag").execute_test().command_is_successful()
