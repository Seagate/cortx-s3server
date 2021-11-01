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
import yaml
import json
import tempfile
import hashlib
import shutil
from framework import Config
from framework import S3PyCliTest
from awss3api import AwsTest
from s3client_config import S3ClientConfig
from aclvalidation import AclTest
from auth import AuthTest
from ldap_setup import LdapInfo
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

def get_etag(response):
    key_pairs = response.split('\t')
    return key_pairs[3]

def load_test_config():
    conf_file = os.path.join(os.path.dirname(__file__),'s3iamcli_test_config.yaml')
    with open(conf_file, 'r') as f:
            config = yaml.safe_load(f)
            S3ClientConfig.ldapuser = config['ldapuser']
            S3ClientConfig.ldappasswd = config['ldappasswd']

def get_response_elements(response):
    response_elements = {}
    key_pairs = response.split(',')

    for key_pair in key_pairs:
        tokens = key_pair.split('=')
        response_elements[tokens[0].strip()] = tokens[1].strip()

    return response_elements

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

def get_md5_sum(fname):
    hash_md5 = hashlib.md5()
    with open(fname, "rb") as f:
        for chunk in iter(lambda: f.read(4096), b""):
            hash_md5.update(chunk)
    return hash_md5.hexdigest()

#******** Create Bucket ********
AwsTest('Aws can create bucket').create_bucket("copyobjectbucket").execute_test().command_is_successful()

# creating dir for files.
upload_dir = os.path.join(os.path.dirname(os.path.realpath(__file__)), "copy_object_test_upload")
os.makedirs(upload_dir, exist_ok=True)
filesize = [1024, 5120, 32768]
start = 0

# Create root level files under upload_dir, also calculating the checksum of files.
md5sum_dict_before_upload = {}
file_list_before_upload = ['1kfile', '5kfile', '32kfile']
for root_file in file_list_before_upload:
    file_to_create = os.path.join(upload_dir, root_file)
    key = root_file
    with open(file_to_create, 'wb+') as fout:
        fout.write(os.urandom(filesize[start]))
    md5sum_dict_before_upload[root_file] = get_md5_sum(file_to_create)
    start = start + 1

#Recursively upload all files from upload_dir to 'copyobjectbucket'.
AwsTest('Aws Upload folder recursively').upload_objects("copyobjectbucket", upload_dir)\
        .execute_test().command_is_successful()

# Create a new destination bucket.
AwsTest('Aws can create destination bucket for CopyObject API')\
    .create_bucket("destination-bucket")\
    .execute_test().command_is_successful()

#copy all objects to destination bucket using CopyObject API.
for file in file_list_before_upload:
    filename = "copyobjectbucket/" + file
    AwsTest('Aws can copy object to different bucket')\
    .copy_object(filename, "destination-bucket", file)\
    .execute_test().command_is_successful()

#creating download_dir to download objects.
download_dir = os.path.join(os.path.dirname(os.path.realpath(__file__)), "copy_object_test_download")
os.makedirs(download_dir, exist_ok=True)

#Download files from destination bucket to local download_dir.
AwsTest('Aws can Download folder recursively').download_objects("destination-bucket", download_dir)\
        .execute_test().command_is_successful()

#getting files from download_dir.
file_list_after_download = os.listdir(download_dir)

print("***************Checksum test start****************")
corruption = False

#calculating md5sum after downloading the objects, also comparing the checksums of files.
md5sum_dict_after_download = {}
for file in file_list_after_download:
    path = os.path.join(download_dir, file)
    md5sum_dict_after_download[file] = get_md5_sum(path)
    if md5sum_dict_before_upload[file] != md5sum_dict_after_download[file]:
        corruption = True
        break

#deleting all objects from source bucket.
for file in file_list_before_upload:
    AwsTest('Aws can delete '+ file + ' from source bucket').delete_object("copyobjectbucket", file)\
    .execute_test().command_is_successful()

#deleting source bucket.
AwsTest('Aws can delete source bucket').delete_bucket("copyobjectbucket")\
    .execute_test().command_is_successful()

#deleting all objects from destination bucket.
for file in file_list_before_upload:
    AwsTest('Aws can delete '+ file + ' from destination bucket').delete_object("destination-bucket", file)\
    .execute_test().command_is_successful()

#deleting destination bucket.
AwsTest('Aws can delete destination bucket').delete_bucket("destination-bucket")\
    .execute_test().command_is_successful()

if corruption:
    assert False, "***************Checksum test Failed****************"
else:
    print("***************Checksum test successful ****************")


#removing the directories.
shutil.rmtree(upload_dir)
shutil.rmtree(download_dir)

#******** Create Bucket ********
AwsTest('Aws can create bucket').create_bucket("seagatebucket").execute_test().command_is_successful()

# Ensure no "Error during pagination: The same next token was received twice:" is seen in ListObjectsV2.
# If command is successful, it ensures no error, inlcuding pagination error.
# Create 2 objects with name containing:
#  loadgen_test_3XQEX2S6WFEDA4B32AJ3T6FAR4XVNKRAE6JET2LSJFFVACUV4MOMLCSH42RXFX22IXYWZS3YFFWYUJE4STNJPGFDOLIDLTQDZ6J2QLA_
# The above key string is specifically chosen, as issue 'Error during pagination' was seen when the key was similar to such string.
obj_list = []
for x in range(2):
    key = "loadgen_test_3XQEX2S6WFEDA4B32AJ3T6FAR4XVNKRAE6JET2LSJFFVACUV4MOMLCSH42RXFX22IXYWZS3YFFWYUJE4STNJPGFDOLIDLTQDZ6J2QLA_%d" % (x)
    AwsTest(('Aws Upload object: %s' % key)).put_object("seagatebucket", "3Kfile", 3000, key_name=key)\
        .execute_test().command_is_successful()
    obj_list.append(key)

#  Create object list file
object_list_file = create_object_list_file("obj_list_v2_keys.json", obj_list, "true")

# List objects using list-objects-v2, page size 7 (first page will have 7 keys, and next page 3 keys)
list_v2_options = {'page-size':1}
result = AwsTest(('Aws list objects using page size \'%d\'') % (list_v2_options['page-size']))\
    .list_objects_v2("seagatebucket", **list_v2_options)\
    .execute_test()\
    .command_is_successful()

# Delete all created objects in bucket 'seagatebucket'
AwsTest('Aws delete objects')\
    .delete_multiple_objects("seagatebucket", object_list_file)\
    .execute_test()\
    .command_is_successful()\
    .command_response_should_be_empty()

delete_object_list_file(object_list_file)

'''
Create following keys (both, regular and heirarchical) into a bucket:
----
asdf
bo0
boo/0...boo/5
boo#
boo+
fo0
foo/0...foo/32
foo#123
foo+123
qua0
quax/0...quax/10
quax#
quax+
-----
'''
# Step 0: Create directory with above structure.
# Create temporary 's3testupload' directory
obj_list = []
temp_dir = os.path.join(os.path.dirname(os.path.realpath(__file__)), "s3testupload")
os.makedirs(temp_dir, exist_ok=True)
# Clear contents of 's3testupload' directory
upload_content = temp_dir + "/*"
os.system("rm -rf " + upload_content)
# List of directories to create
dirs = ['boo', 'foo', 'quax']
filesize = 1024
object_range = [5, 32, 10]
index = 0 
for child_dir in dirs:
    out_dir = os.path.join(temp_dir, child_dir)
    os.makedirs(out_dir, exist_ok=True)
    # Create files within directory
    for i in range(object_range[index]):
        filename = str(i)
        key = "%s/%s" % (child_dir, filename)
        file_to_create = os.path.join(out_dir, filename)
        with open(file_to_create, 'wb+') as fout:
            fout.write(os.urandom(filesize))
        obj_list.append(key)
    index = index + 1

# Create remaining root level files
file_list = ['asdf', 'bo0', 'boo#', 'boo+', 'fo0', 'foo#123', 'foo+123', 'qua0', 'quax#', 'quax+']
for root_file in file_list:
    file_to_create = os.path.join(temp_dir, root_file)
    key = root_file
    with open(file_to_create, 'wb+') as fout:
        fout.write(os.urandom(filesize))
    obj_list.append(key)

# Step 1: Upload folder recrusively.
#  Step 1.1:
#   Create above keys into bucket 'seagatebucket'
AwsTest('Aws Upload folder recursively').upload_objects("seagatebucket", temp_dir)\
        .execute_test().command_is_successful()

#  Step 1.2: Create object list file
object_list_file = create_object_list_file("obj_list_mix_keys.json", obj_list, "true")

# Step 2: Validate the uploaded objects using s3api
# Step 2.1: command:= aws s3api list-objects-v2 --bucket <bucket> --page-size 1 --prefix "foo" --delimiter "/"
# Expected output: 
#   foo#123
#   foo+123
#   foo/ (under COMMONPREFIXES)
prefix = "foo"
delimiter = "/"
expected_key_list = ['foo#123', 'foo+123']
list_v2_options = {"prefix":prefix, "delimiter":delimiter,"page-size":1}
result = AwsTest(('Aws list objects using prefix \'%s\' and delimiter %s') % (prefix, delimiter))\
    .list_objects_v2("seagatebucket", **list_v2_options)\
    .execute_test()\
    .command_is_successful()
# Process result set
lv2_response = get_aws_cli_object(result.status.stdout)
if lv2_response is None:
    assert False, "Failed to list objects"
# Get keys from lv2_response
obj_keys = lv2_response["keys"]
common_prefix = lv2_response["prefix"]
check = all(item in obj_keys for item in expected_key_list)
if check is False:
    assert False, "Failed to match expected regular keys in the list"
check = ("foo/" in common_prefix)
if check is False:
    assert False, "Failed to match expected common prefix in the list"

# Step 2.2: command:= aws s3api list-objects-v2 --bucket <bucket> --page-size 2 --delimiter "/"
# Expected output: 
#  asdf
#  bo0
#  boo#
#  boo+
#  fo0
#  foo#123
#  foo+123
#  qua0
#  quax#
#  quax+
#  <items under common prefixes are as follows>
#  boo/
#  foo/
#  quax/
prefix = ""
expected_key_list = ['asdf', 'bo0', 'boo#', 'boo+', 'fo0', 'foo#123', 'foo+123', 'qua0', 'quax#', 'quax+']
expected_common_prefix = ['boo/', 'foo/', 'quax/']
list_v2_options = {"delimiter":delimiter, "page-size":2}
result = AwsTest(('Aws list objects using prefix \'%s\' and delimiter %s') % (prefix, delimiter))\
    .list_objects_v2("seagatebucket", **list_v2_options)\
    .execute_test()\
    .command_is_successful()
# Process result set
lv2_response = get_aws_cli_object(result.status.stdout)
if lv2_response is None:
    assert False, "Failed to list objects"
# Get keys from lv2_response
obj_keys = lv2_response["keys"]
common_prefix = lv2_response["prefix"]
check = all(item in obj_keys for item in expected_key_list)
if check is False:
    assert False, "Failed to match expected regular keys in the list"
check = all(item in common_prefix for item in expected_common_prefix)
if check is False:
    assert False, "Failed to match expected common prefix in the list"

# Step 2.3.1: command:= aws s3api list-objects-v2 --bucket <bucket> --max-items 1 --prefix "boo" --delimiter "/"
# Expected part1 output:
#  boo#
#  boo/ (under common prefix)
prefix = "boo"
expected_key_list_part1 = ['boo#']
expected_key_list_part2 = ['boo+']
expected_common_prefix = ['boo/']
list_v2_options = {"prefix":prefix, "delimiter":delimiter, "max-items":1}
result = AwsTest(('Aws list objects using prefix \'%s\' and delimiter %s') % (prefix, delimiter))\
    .list_objects_v2("seagatebucket", **list_v2_options)\
    .execute_test()\
    .command_is_successful()
# Process result set
lv2_response = get_aws_cli_object(result.status.stdout)
if lv2_response is None:
    assert False, "Failed to list objects"
# Get keys from lv2_response
obj_keys = lv2_response["keys"]
common_prefix = lv2_response["prefix"]
check = all(item in obj_keys for item in expected_key_list_part1)
if check is False:
    assert False, "Failed to match expected regular keys in the list"
check = all(item in common_prefix for item in expected_common_prefix)
if check is False:
    assert False, "Failed to match expected common prefix in the list"
# Verify that NextToken is in 'lv2_response' and it is not empty
assert (("next_token" in lv2_response.keys()) and (len(lv2_response["next_token"].strip()) > 0)), "NextToken is either not present or empty"
next_token = lv2_response["next_token"]

# Step 2.3.2: command:= aws s3api list-objects-v2 --bucket <bucket> --max-items 2 --prefix "boo" --delimiter "/" --starting-token <next_token>
# Expected part2 output:
#  boo+
list_v2_options = {"prefix":prefix, "delimiter":delimiter, "max-items":2, "starting-token":next_token}
result = AwsTest(('Aws list objects using prefix \'%s\' and delimiter %s') % (prefix, delimiter))\
    .list_objects_v2("seagatebucket", **list_v2_options)\
    .execute_test()\
    .command_is_successful()
# Process result set
lv2_response = get_aws_cli_object(result.status.stdout)
if lv2_response is None:
    assert False, "Failed to list objects"
# Get keys from lv2_response
obj_keys = lv2_response["keys"]
check = all(item in obj_keys for item in expected_key_list_part2)
if check is False:
    assert False, "Failed to match expected regular keys in the list"

# Step 3: Delete all created objects in bucket 'seagatebucket'
AwsTest('Aws delete objects')\
    .delete_multiple_objects("seagatebucket", object_list_file)\
    .execute_test()\
    .command_is_successful()\
    .command_response_should_be_empty()

delete_object_list_file(object_list_file)
os.system("rm -rf " + upload_content)


#******** Verify pagination and NextToken in List objects V1  **********
# Step 1. Create total 35 objets in bucket
   # Create files within directory
obj_35_list = []
filesize = 1024
for i in range(35):
    key = "object%s" % (str(i))
    file_to_create = os.path.join(temp_dir, key)
    with open(file_to_create, 'wb+') as fout:
        fout.write(os.urandom(filesize))
    obj_35_list.append(key)

# Step 1: Upload folder recrusively.
#  Create above keys into bucket 'seagatebucket'
AwsTest('Aws Upload folder recursively').upload_objects("seagatebucket", temp_dir)\
        .execute_test().command_is_successful()
# Step 2.
# Create object list file
object_list_file = create_object_list_file("obj_list_35_keys.json", obj_35_list, "true")

# Step 3. List first 20 objects. This will result into NextToken != None
# command: aws s3api list-objects --bucket <bucket> --max-items 20
# NextToken will point to object26 - the last object key in the response.
result = AwsTest('Aws list the first 20 objects')\
    .list_objects("seagatebucket", None, "20")\
    .execute_test()\
    .command_is_successful()

#Step 4 Validation
# Step 4.1 Validate presence of NextToken in response
list_response = get_aws_cli_object(result.status.stdout)
# Verify 'object26' is present and 'object27' is not present in the resultant list 'list_response'
if list_response is not None:
    assert 'object26' in list_response["keys"], "Failed to see object26 in the response"
    assert 'object27' not in list_response["keys"], "Failed: Unexpected key object27 in the response"
else:
    assert False, "Failed to list objects from bucket"
# Verify that NextToken is in 'list_response' and it is not empty
assert (("next_token" in list_response.keys()) and (len(list_response["next_token"].strip()) > 0)), "NextToken is either not present or empty"

# Step 4.2 Validate response using starting token
next_token = list_response["next_token"]
# command:= aws s3api list-objects --bucket <bucket> --starting-token <next_token>
# Expected output:
#   keys = ['object27', 'object28', 'object29', 'object3', 'object30', 'object31', 'object32', 'object33', \
#   'object34', 'object4', 'object5', 'object6', 'object7', 'object8', 'object9']
expected_key_list_part2 = ['object27', 'object28', 'object29', 'object3', 'object30', 'object31', 'object32', 'object33', \
    'object34', 'object4', 'object5', 'object6', 'object7', 'object8', 'object9']
list_options = {"starting-token":next_token}
result = AwsTest(('Aws list objects using starting token:%s') % (next_token))\
    .list_objects("seagatebucket", starting_token=next_token)\
    .execute_test()\
    .command_is_successful()
# Process result set
lv1_response = get_aws_cli_object(result.status.stdout)
if lv1_response is None:
    assert False, "Failed to list objects"
# Get keys from response
obj_keys = lv1_response["keys"]
check = all(item in obj_keys for item in expected_key_list_part2)
if check is False:
    assert False, "Failed to match expected keys in the list\n:%s" % str(obj_keys)

# Step 5. Cleanup: Delete all 35 objects, and remove object list file containing a list of 35 objects
# Delete all 35 objects in bucket
AwsTest('Aws delete objects: [object0...object35]')\
    .delete_multiple_objects("seagatebucket", object_list_file)\
    .execute_test()\
    .command_is_successful()\
    .command_response_should_be_empty()

delete_object_list_file(object_list_file)
os.system("rm -rf " + upload_content)
os.rmdir(temp_dir)

#*********************Range Read***************************
#Creating a multipart upload
AwsTest('Aws can create bucket').create_bucket("rangereadbucket").execute_test().command_is_successful()

result=AwsTest('Aws can upload 10Mb file with tags').create_multipart_upload("rangereadbucket", "10Mbfile", 10485760, "domain=storage" ).execute_test().command_is_successful()
upload_id = get_upload_id(result.status.stdout)
print(upload_id)

result=AwsTest('Aws can upload 5Mb first part').upload_part("rangereadbucket", "firstpart", 5242880, "10Mbfile", "1" , upload_id).execute_test().command_is_successful()
e_tag_1 = result.status.stdout
print(e_tag_1)

result=AwsTest('Aws can upload 5Mb second part').upload_part("rangereadbucket", "secondpart", 5242880, "10Mbfile", "2" , upload_id).execute_test().command_is_successful()
e_tag_2 = result.status.stdout
print(e_tag_2)

parts="Parts=[{ETag="+e_tag_1.strip('\n')+",PartNumber="+str(1)+"},{ETag="+e_tag_2.strip('\n')+",PartNumber="+str(2)+"}]"
print(parts)

result=AwsTest('Aws can complete multipart upload 10Mb file with tags').complete_multipart_upload("rangereadbucket", "10Mbfile", parts, upload_id).execute_test().command_is_successful().command_response_should_have("rangereadbucket/10Mbfile")

# Range read after multipart upload such that the range is within a part
AwsTest('Aws can get object').get_object("rangereadbucket", "10Mbfile", start_range = '0', end_range = '1000000')\
    .execute_test().command_is_successful()

# Range read after multipart upload such that the range is across parts
AwsTest('Aws can get object').get_object("rangereadbucket", "10Mbfile", start_range = '2000000', end_range = '7242880')\
    .execute_test().command_is_successful()

# Range read after multipart upload such that the range is exactly a part
AwsTest('Aws can get object').get_object("rangereadbucket", "10Mbfile", start_range = '0', end_range = '5242879')\
    .execute_test().command_is_successful()

# Range read after multipart upload such that the range is complete object itself
AwsTest('Aws can get object').get_object("rangereadbucket", "10Mbfile", start_range = '0', end_range = '10485759')\
    .execute_test().command_is_successful()

AwsTest('Aws can delete sourceobj').delete_object("rangereadbucket", "10Mbfile").execute_test().command_is_successful()

AwsTest('Aws can delete source bucket').delete_bucket("rangereadbucket")\
    .execute_test().command_is_successful()

#********************Deleting bucket after multiple object uploads********************
#negative case

#for multiple simple objects in a bucket
AwsTest('Aws can create bucket').create_bucket("simple-bucket").execute_test().command_is_successful()

AwsTest('Aws can put simple object').put_object("simple-bucket", "simpleobj1", 1024)\
    .execute_test().command_is_successful()

AwsTest('Aws can put simple object').put_object("simple-bucket", "simpleobj2", 1024)\
    .execute_test().command_is_successful()

AwsTest('Aws can put simple object').put_object("simple-bucket", "simpleobj3", 1024)\
    .execute_test().command_is_successful()

AwsTest('Aws cannot delete source bucket since the bucket is not empty')\
    .delete_bucket("simple-bucket")\
    .execute_test(negative_case=True).command_should_fail().command_error_should_have("BucketNotEmpty")

#for multiple multipart objects in a bucket
#creating a bucket
AwsTest('Aws can create bucket').create_bucket("multipart-bucket").execute_test().command_is_successful()

#Creating a first multipart upload
result=AwsTest('Aws can upload 10Mb file with tags').create_multipart_upload("multipart-bucket", "10Mbfile1", 10485760, "domain=storage" ).execute_test().command_is_successful()
upload_id = get_upload_id(result.status.stdout)
print(upload_id)

result=AwsTest('Aws can upload 5Mb first part').upload_part("multipart-bucket", "firstpart", 5242880, "10Mbfile1", "1" , upload_id).execute_test().command_is_successful()
e_tag_1 = result.status.stdout
print(e_tag_1)

result=AwsTest('Aws can upload 5Mb second part').upload_part("multipart-bucket", "secondpart", 5242880, "10Mbfile1", "2" , upload_id).execute_test().command_is_successful()
e_tag_2 = result.status.stdout
print(e_tag_2)

parts="Parts=[{ETag="+e_tag_1.strip('\n')+",PartNumber="+str(1)+"},{ETag="+e_tag_2.strip('\n')+",PartNumber="+str(2)+"}]"
print(parts)

result=AwsTest('Aws can complete multipart upload 10Mb file with tags').complete_multipart_upload("multipart-bucket", "10Mbfile1", parts, upload_id).execute_test().command_is_successful().command_response_should_have("multipart-bucket/10Mbfile1")

#Creating a second multipart upload
result=AwsTest('Aws can upload 10Mb file with tags').create_multipart_upload("multipart-bucket", "10Mbfile2", 10485760, "domain=storage" ).execute_test().command_is_successful()
upload_id = get_upload_id(result.status.stdout)
print(upload_id)

result=AwsTest('Aws can upload 5Mb first part').upload_part("multipart-bucket", "firstpart", 5242880, "10Mbfile2", "1" , upload_id).execute_test().command_is_successful()
e_tag_1 = result.status.stdout
print(e_tag_1)

result=AwsTest('Aws can upload 5Mb second part').upload_part("multipart-bucket", "secondpart", 5242880, "10Mbfile2", "2" , upload_id).execute_test().command_is_successful()
e_tag_2 = result.status.stdout
print(e_tag_2)

parts="Parts=[{ETag="+e_tag_1.strip('\n')+",PartNumber="+str(1)+"},{ETag="+e_tag_2.strip('\n')+",PartNumber="+str(2)+"}]"
print(parts)

result=AwsTest('Aws can complete multipart upload 10Mb file with tags').complete_multipart_upload("multipart-bucket", "10Mbfile2", parts, upload_id).execute_test().command_is_successful().command_response_should_have("multipart-bucket/10Mbfile2")

AwsTest('Aws cannot delete source bucket')\
    .delete_bucket("multipart-bucket")\
    .execute_test(negative_case=True).command_should_fail().command_error_should_have("BucketNotEmpty")

#deleting a bucket after deleting objects(simple object case)
AwsTest('Aws can delete sourceobj').delete_object("simple-bucket", "simpleobj1").execute_test().command_is_successful()

AwsTest('Aws can delete sourceobj').delete_object("simple-bucket", "simpleobj2").execute_test().command_is_successful()

AwsTest('Aws can delete sourceobj').delete_object("simple-bucket", "simpleobj3").execute_test().command_is_successful()

AwsTest('Aws can delete source bucket').delete_bucket("simple-bucket")\
    .execute_test().command_is_successful()

#deleting a bucket after deleting objects(multipart object case)
AwsTest('Aws can delete sourceobj').delete_object("multipart-bucket", "10Mbfile1").execute_test().command_is_successful()

AwsTest('Aws can delete sourceobj').delete_object("multipart-bucket", "10Mbfile2").execute_test().command_is_successful()

AwsTest('Aws can delete source bucket').delete_bucket("multipart-bucket")\
    .execute_test().command_is_successful()


# **************** Multipart CopyObject API supported *********************************
AwsTest('Aws can create bucket').create_bucket("source-bucket").execute_test().command_is_successful()

# Aligned Copy
#************** Create a multipart upload ********
result=AwsTest('Aws can upload 20Mb file with tags').create_multipart_upload("source-bucket", "20Mbfile", 20971520, "domain=storage" ).execute_test().command_is_successful()
upload_id = get_upload_id(result.status.stdout)
print(upload_id)

#************** Upload Individual parts ********
result=AwsTest('Aws can upload 10Mb first part').upload_part("source-bucket", "firstpart", 10485760, "20Mbfile", "1" , upload_id).execute_test().command_is_successful()
e_tag_1 = result.status.stdout
print(e_tag_1)

result=AwsTest('Aws can upload 10Mb second part').upload_part("source-bucket", "secondpart", 10485760, "20Mbfile", "2" , upload_id).execute_test().command_is_successful()
e_tag_2 = result.status.stdout
print(e_tag_2)

parts="Parts=[{ETag="+e_tag_1.strip('\n')+",PartNumber="+str(1)+"},{ETag="+e_tag_2.strip('\n')+",PartNumber="+str(2)+"}]"
print(parts)

#************** Get object acl should fail before complete multipart upload ******
AwsTest('Aws can get object acl').get_object_acl("source-bucket", "20Mbfile").execute_test(negative_case=True)\
.command_should_fail().command_error_should_have("NoSuchKey")

#************** Complete multipart upload ********
result=AwsTest('Aws can complete multipart upload 20Mb file with tags').complete_multipart_upload("source-bucket", "20Mbfile", parts, upload_id).execute_test().command_is_successful().command_response_should_have("source-bucket/20Mbfile")

# Positive: copy multipart object to different destination bucket.
AwsTest('Aws can create destination bucket for Multipart CopyObject API')\
    .create_bucket("destination-bucket")\
    .execute_test().command_is_successful()

AwsTest('Aws can copy object to different bucket')\
    .copy_object("source-bucket/20Mbfile", "destination-bucket", "20Mbfile-copy")\
    .execute_test().command_is_successful().command_response_should_have("COPYOBJECTRESULT")

# Positive: copy multipart object to same destination bucket with different name.
AwsTest('Aws can copy object to same bucket')\
    .copy_object("source-bucket/20Mbfile", "source-bucket", "20Mbfile-copy")\
    .execute_test().command_is_successful().command_response_should_have("COPYOBJECTRESULT")

#get-object after copying and validating Etag
dest_obj_data = AwsTest('Aws can get object').get_object("destination-bucket", "20Mbfile-copy").execute_test().command_is_successful()
dest_obj_etag = get_etag(dest_obj_data.status.stdout)
print(dest_obj_etag)

source_obj_data = AwsTest('Aws can get object').get_object("source-bucket", "20Mbfile-copy").execute_test().command_is_successful()
source_obj_etag = get_etag(source_obj_data.status.stdout)
print(source_obj_etag)

if dest_obj_etag != source_obj_etag:
    assert False, "Copy object Tests Failed***************"

# *******************************UnAligned Copy********************************************
#************** Create a multipart upload ********
result=AwsTest('Aws can upload 10.4Mb file with tags').create_multipart_upload("source-bucket", "10.4Mbfile", 10905190, "domain=storage" ).execute_test().command_is_successful()
upload_id = get_upload_id(result.status.stdout)
print(upload_id)

#************** Upload Individual parts ********
result=AwsTest('Aws can upload 5.2Mb first part').upload_part("source-bucket", "firstpart", 5452595, "10.4Mbfile", "1" , upload_id).execute_test().command_is_successful()
e_tag_1 = result.status.stdout
print(e_tag_1)

result=AwsTest('Aws can upload 5.2Mb second part').upload_part("source-bucket", "secondpart", 5452595, "10.4Mbfile", "2" , upload_id).execute_test().command_is_successful()
e_tag_2 = result.status.stdout
print(e_tag_2)

parts="Parts=[{ETag="+e_tag_1.strip('\n')+",PartNumber="+str(1)+"},{ETag="+e_tag_2.strip('\n')+",PartNumber="+str(2)+"}]"
print(parts)

#************** Complete multipart upload ********
result=AwsTest('Aws can complete multipart upload 10.4Mb file with tags').complete_multipart_upload("source-bucket", "10.4Mbfile", parts, upload_id).execute_test().command_is_successful().command_response_should_have("source-bucket/10.4Mbfile")

# Positive: copy multipart object to different destination bucket.
AwsTest('Aws can copy object to different bucket')\
    .copy_object("source-bucket/10.4Mbfile", "destination-bucket", "10.4Mbfile-copy")\
    .execute_test().command_is_successful().command_response_should_have("COPYOBJECTRESULT")

# Positive: copy multipart object to same destination bucket with different name.
AwsTest('Aws can copy object to same bucket')\
    .copy_object("source-bucket/10.4Mbfile", "source-bucket", "10.4Mbfile-copy")\
    .execute_test().command_is_successful().command_response_should_have("COPYOBJECTRESULT")

#get-object after copying and validating Etag
dest_obj_data = AwsTest('Aws can get object').get_object("destination-bucket", "10.4Mbfile-copy").execute_test().command_is_successful()
dest_obj_etag = get_etag(dest_obj_data.status.stdout)
print(dest_obj_etag)

source_obj_data = AwsTest('Aws can get object').get_object("source-bucket", "10.4Mbfile-copy").execute_test().command_is_successful()
source_obj_etag = get_etag(source_obj_data.status.stdout)
print(source_obj_etag)

if dest_obj_etag != source_obj_etag:
    assert False, "Copy object Tests Failed***************"

# ***************Overwrite case*******************
# A ---> multipart object upload
# B ---> Simple object 
# Four cases : (overwrite second one with first)
# AA, AB, BA, BB

#CASE 1 : AA (overwrite multipart object with multipart object)

#************** Create a multipart upload ********
result=AwsTest('Aws can upload 10Mb file with tags').create_multipart_upload("source-bucket", "10MbAAfile", 10485760, "domain=storage" ).execute_test().command_is_successful()
upload_id = get_upload_id(result.status.stdout)
print(upload_id)

#************** Upload Individual parts ********
result=AwsTest('Aws can upload 5Mb first part').upload_part("source-bucket", "firstpart", 5242880, "10MbAAfile", "1" , upload_id).execute_test().command_is_successful()
e_tag_1 = result.status.stdout
print(e_tag_1)

result=AwsTest('Aws can upload 5Mb second part').upload_part("source-bucket", "secondpart", 5242880, "10MbAAfile", "2" , upload_id).execute_test().command_is_successful()
e_tag_2 = result.status.stdout
print(e_tag_2)

parts="Parts=[{ETag="+e_tag_1.strip('\n')+",PartNumber="+str(1)+"},{ETag="+e_tag_2.strip('\n')+",PartNumber="+str(2)+"}]"
print(parts)

#************** Complete multipart upload ********
result=AwsTest('Aws can complete multipart upload 10Mb file with tags').complete_multipart_upload("source-bucket", "10MbAAfile", parts, upload_id).execute_test().command_is_successful().command_response_should_have("source-bucket/10MbAAfile")

#overwrite with the same multipart object
#************** Create a multipart upload ********
result=AwsTest('Aws can upload 10Mb file with tags').create_multipart_upload("source-bucket", "10MbAAfile", 10485760, "domain=storage" ).execute_test().command_is_successful()
upload_id = get_upload_id(result.status.stdout)
print(upload_id)

#************** Upload Individual parts ********
result=AwsTest('Aws can upload 5Mb first part').upload_part("source-bucket", "firstpart", 5242880, "10MbAAfile", "1" , upload_id).execute_test().command_is_successful()
e_tag_1 = result.status.stdout
print(e_tag_1)

result=AwsTest('Aws can upload 5Mb second part').upload_part("source-bucket", "secondpart", 5242880, "10MbAAfile", "2" , upload_id).execute_test().command_is_successful()
e_tag_2 = result.status.stdout
print(e_tag_2)

parts="Parts=[{ETag="+e_tag_1.strip('\n')+",PartNumber="+str(1)+"},{ETag="+e_tag_2.strip('\n')+",PartNumber="+str(2)+"}]"
print(parts)

#************** Complete multipart upload ********
result=AwsTest('Aws can complete multipart upload 10Mb file with tags').complete_multipart_upload("source-bucket", "10MbAAfile", parts, upload_id).execute_test().command_is_successful().command_response_should_have("source-bucket/10MbAAfile")

#get-object after overwrite
AwsTest('Aws can get object').get_object("source-bucket", "10MbAAfile").execute_test().command_is_successful()

#CASE 2 : AB (overwrite simple object with multipart object)
#simple object upload
AwsTest('Aws can put object').put_object("source-bucket", "10MbABfile", 1024)\
    .execute_test().command_is_successful()

#overwrite with the multipart object
#************** Create a multipart upload ********
result=AwsTest('Aws can upload 10Mb file with tags').create_multipart_upload("source-bucket", "10MbABfile", 10485760, "domain=storage" ).execute_test().command_is_successful()
upload_id = get_upload_id(result.status.stdout)
print(upload_id)

#************** Upload Individual parts ********
result=AwsTest('Aws can upload 5Mb first part').upload_part("source-bucket", "firstpart", 5242880, "10MbABfile", "1" , upload_id).execute_test().command_is_successful()
e_tag_1 = result.status.stdout
print(e_tag_1)

result=AwsTest('Aws can upload 5Mb second part').upload_part("source-bucket", "secondpart", 5242880, "10MbABfile", "2" , upload_id).execute_test().command_is_successful()
e_tag_2 = result.status.stdout
print(e_tag_2)

parts="Parts=[{ETag="+e_tag_1.strip('\n')+",PartNumber="+str(1)+"},{ETag="+e_tag_2.strip('\n')+",PartNumber="+str(2)+"}]"
print(parts)

#************** Complete multipart upload ********
result=AwsTest('Aws can complete multipart upload 10Mb file with tags').complete_multipart_upload("source-bucket", "10MbABfile", parts, upload_id).execute_test().command_is_successful().command_response_should_have("source-bucket/10MbABfile")

#get-object after overwrite
AwsTest('Aws can get object').get_object("source-bucket", "10MbABfile").execute_test().command_is_successful()

#CASE 3 : BA (overwrite multipart object with simple object)
#multipart object upload
#************** Create a multipart upload ********
result=AwsTest('Aws can upload 10Mb file with tags').create_multipart_upload("source-bucket", "10MbBAfile", 10485760, "domain=storage" ).execute_test().command_is_successful()
upload_id = get_upload_id(result.status.stdout)
print(upload_id)

#************** Upload Individual parts ********
result=AwsTest('Aws can upload 5Mb first part').upload_part("source-bucket", "firstpart", 5242880, "10MbBAfile", "1" , upload_id).execute_test().command_is_successful()
e_tag_1 = result.status.stdout
print(e_tag_1)

result=AwsTest('Aws can upload 5Mb second part').upload_part("source-bucket", "secondpart", 5242880, "10MbBAfile", "2" , upload_id).execute_test().command_is_successful()
e_tag_2 = result.status.stdout
print(e_tag_2)

parts="Parts=[{ETag="+e_tag_1.strip('\n')+",PartNumber="+str(1)+"},{ETag="+e_tag_2.strip('\n')+",PartNumber="+str(2)+"}]"
print(parts)

#************** Complete multipart upload ********
result=AwsTest('Aws can complete multipart upload 10Mb file with tags').complete_multipart_upload("source-bucket", "10MbBAfile", parts, upload_id).execute_test().command_is_successful().command_response_should_have("source-bucket/10MbBAfile")

#overwrite with simple object upload
AwsTest('Aws can put object').put_object("source-bucket", "10MbBAfile", 1024)\
    .execute_test().command_is_successful()

#get-object after overwrite
AwsTest('Aws can get object').get_object("source-bucket", "10MbBAfile").execute_test().command_is_successful()

#CASE 4 : BB (overwrite simple object with simple object)
#simple object upload
AwsTest('Aws can put object').put_object("source-bucket", "10MbBBfile", 1024)\
    .execute_test().command_is_successful()

#overwrite with simple object upload
AwsTest('Aws can put object').put_object("source-bucket", "10MbBBfile", 1024)\
    .execute_test().command_is_successful()

#get-object after overwrite
AwsTest('Aws can get object').get_object("source-bucket", "10MbBBfile").execute_test().command_is_successful()

# Delete source and destination buckets and objects from them.
AwsTest('Aws can delete sourceobj').delete_object("source-bucket", "10MbAAfile").execute_test().command_is_successful()

AwsTest('Aws can delete sourceobj').delete_object("source-bucket", "10MbABfile").execute_test().command_is_successful()

AwsTest('Aws can delete sourceobj').delete_object("source-bucket", "10MbBAfile").execute_test().command_is_successful()

AwsTest('Aws can delete sourceobj').delete_object("source-bucket", "10MbBBfile").execute_test().command_is_successful()

AwsTest('Aws can delete sourceobj').delete_object("source-bucket", "20Mbfile").execute_test().command_is_successful()

AwsTest('Aws can delete sourceobj').delete_object("source-bucket", "20Mbfile-copy").execute_test().command_is_successful()

AwsTest('Aws can delete destinationobj').delete_object("destination-bucket", "20Mbfile-copy").execute_test().command_is_successful()

AwsTest('Aws can delete sourceobj').delete_object("source-bucket", "10.4Mbfile").execute_test().command_is_successful()

AwsTest('Aws can delete sourceobj').delete_object("source-bucket", "10.4Mbfile-copy").execute_test().command_is_successful()

AwsTest('Aws can delete destinationobj').delete_object("destination-bucket", "10.4Mbfile-copy").execute_test().command_is_successful()

AwsTest('Aws can delete source bucket').delete_bucket("source-bucket").execute_test().command_is_successful()

AwsTest('Aws can delete destination bucket').delete_bucket("destination-bucket").execute_test().command_is_successful()

# ************ CopyObject API supported *****************************************************************

AwsTest('Aws can create bucket').create_bucket("sourcebucket").execute_test().command_is_successful()
AwsTest('Aws can create object with canned acl input').put_object("sourcebucket", "sourceobj", canned_acl="public-read").execute_test().command_is_successful()

test_msg = "Create account testAcc2"
account_args = {'AccountName': 'testAcc2', 'Email': 'testAcc2@seagate.com', 'ldapuser': "sgiamadmin", 'ldappasswd': LdapInfo.get_ldap_admin_pwd()}
account_response_pattern = "AccountId = [\w-]*, CanonicalId = [\w-]*, RootUserName = [\w+=,.@-]*, AccessKeyId = [\w-]*, SecretKey = [\w/+]*$"
result = AuthTest(test_msg).create_account(**account_args).execute_test()
result.command_should_match_pattern(account_response_pattern)
account_response_elements = AuthTest.get_response_elements(result.status.stdout)
testAcc2_access_key = account_response_elements['AccessKeyId']
testAcc2_secret_key = account_response_elements['SecretKey']
testAcc2_cannonicalid = account_response_elements['CanonicalId']
testAcc2_email = "testAcc2@seagate.com"

os.environ["AWS_ACCESS_KEY_ID"] = testAcc2_access_key
os.environ["AWS_SECRET_ACCESS_KEY"] = testAcc2_secret_key

AwsTest('Aws can create bucket').create_bucket("destbucket").execute_test().command_is_successful()

cmd = "curl -s -X PUT -H \"Accept: application/json\" -H \"Content-Type: application/json\" --header 'x-amz-copy-source: /sourcebucket/sourceobj' https://s3.seagate.com/destbucket/destobj --cacert /etc/ssl/stx-s3-clients/s3/ca.crt"
AwsTest('CopyObject not accessible for allusers').execute_curl(cmd).execute_test().command_response_should_have("AccessDenied")

AwsTest('Aws can delete destination bucket').delete_bucket("destbucket").execute_test().command_is_successful()

del os.environ["AWS_ACCESS_KEY_ID"]
del os.environ["AWS_SECRET_ACCESS_KEY"]

AwsTest('Aws can delete sourceobj').delete_object("sourcebucket", "sourceobj").execute_test().command_is_successful()
AwsTest('Aws can delete sourcebucket').delete_bucket("sourcebucket").execute_test().command_is_successful()

S3ClientConfig.access_key_id = testAcc2_access_key
S3ClientConfig.secret_key = testAcc2_secret_key
account_args = {'AccountName': 'testAcc2', 'Email': 'testAcc2@seagate.com',  'force': True}
AuthTest('Delete account testAcc2').delete_account(**account_args).execute_test().command_response_should_have("Account deleted successfully")


AwsTest('Aws can put object').put_object("seagatebucket", "1kfile", 1024)\
    .execute_test().command_is_successful()

# Positive: copy-object from a bucket to the same bucket as destination.
AwsTest('Aws can copy object to same bucket').copy_object("seagatebucket/1kfile", "seagatebucket", "1kfile-copy")\
    .execute_test().command_is_successful().command_response_should_have("COPYOBJECTRESULT")

# Positive: copy-object to different destination bucket.
AwsTest('Aws can create destination bucket for CopyObject API')\
    .create_bucket("destination-seagatebucket")\
    .execute_test().command_is_successful()

AwsTest('Aws can copy object to different bucket')\
    .copy_object("seagatebucket/1kfile", "destination-seagatebucket", "1kfile-copy")\
    .execute_test().command_is_successful().command_response_should_have("COPYOBJECTRESULT")

# Negative: copy-object to non-existing destination bucket
AwsTest('Aws cannot copy object to non-existing destination bucket')\
    .copy_object("seagatebucket/1kfile", "my-bucket", "1kfile-copy")\
    .execute_test(negative_case=True).command_should_fail().command_error_should_have("NoSuchBucket")

# Negative: copy-object from non-existing source bucket
AwsTest('Aws cannot copy object from non-existing source bucket')\
    .copy_object("my-seagate-bucket/1kfile", "seagatebucket", "1kfile-copy")\
    .execute_test(negative_case=True).command_should_fail().command_error_should_have("NoSuchBucket")

# Negative: copy-object from non-existing source object
AwsTest('Aws cannot copy object from non-existing source object')\
    .copy_object("seagatebucket/2kfile", "destination-seagatebucket", "2kfile-copy")\
    .execute_test(negative_case=True).command_should_fail().command_error_should_have("NoSuchKey")

# Negative: copy-object when source and destination are same
AwsTest('Aws cannot copy when source and destintion are same')\
    .copy_object("seagatebucket/1kfile", "seagatebucket", "1kfile")\
    .execute_test(negative_case=True).command_should_fail().command_error_should_have("InvalidRequest")

# Commenting out below test as this fails with memory error :-
# fout.write(os.urandom(self.filesize))
# MemoryError

# Negative: copy-object when source object size is greater than 5GB
# AwsTest('Aws can put 6GB object').put_object("seagatebucket", "6gbfile", 6442450944)\
#    .execute_test().command_is_successful()

# AwsTest('Aws cannot copy 6gb source object').copy_object("seagatebucket/6gbfile", "seagatebucket", "6gbfile-copy")\
#    .execute_test(negative_case=True).command_should_fail().command_error_should_have("InvalidRequest")

# Cleanup for CopyObject API
AwsTest('Aws can delete 1kfile').delete_object("seagatebucket", "1kfile")\
    .execute_test().command_is_successful()

# AwsTest('Aws can delete 6gbfile').delete_object("seagatebucket", "6gbfile")\
#    .execute_test().command_is_successful()

AwsTest('Aws can delete 1kfile-copy from source bucket').delete_object("seagatebucket", "1kfile-copy")\
    .execute_test().command_is_successful()

AwsTest('Aws can delete 1kfile-copy from destination bucket').delete_object("destination-seagatebucket", "1kfile-copy")\
    .execute_test().command_is_successful()

AwsTest('Aws can delete destination bucket').delete_bucket("destination-seagatebucket")\
    .execute_test().command_is_successful()

# **********************************************************************************************

# ************ Put object with specified content-type ******************************************
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


# ********* Verify ListObject V2 hierarchical object key tests  **********
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
#Above test would not abort multipart operation, so need to do it explicitly
result=AwsTest('Aws abort previous multipart upload').abort_multipart_upload("seagatebuckettag", "10Mbfile", upload_id).execute_test(negative_case=True).command_is_successful()

#******* Multipart upload should fail if user completes multipart-upload with wrong ETag ******

result=AwsTest('Multipart upload with wrong ETag').create_multipart_upload("seagatebuckettag", "10Mbfile", 10485760, "domain=storage" ).execute_test().command_is_successful()
upload_id = get_upload_id(result.status.stdout)
print(upload_id)

parts="Parts=[{ETag=00000000000000000000000000000000,PartNumber=1}]"

result=AwsTest('Aws cannot complete multipart upload with wrong ETag').complete_multipart_upload("seagatebuckettag", "10Mbfile", parts, upload_id).execute_test(negative_case=True).command_should_fail().command_error_should_have("InvalidPart")

#******** Bucket Replication ********

REPLICATION_SOURCE = 'replication-source'
REPLICATION_DEST = 'replication-dst'
REPLICATION_POLICY = {
    "Role": "role-string",
    "Rules": [
        {
            "Status": "Enabled",
            "Prefix": "test",
            "Destination": {
                "Bucket": REPLICATION_DEST
            }
        }
    ]
}

def put_replication(bucket, policy):
    with tempfile.NamedTemporaryFile(suffix='.json', mode='w') as f:
        json.dump(policy, f)
        f.flush()
        os.fsync(f.fileno())
        AwsTest('Put bucket replication').put_bucket_replication(bucket, f.name).execute_test().command_is_successful()

AwsTest('Make replication source').create_bucket(REPLICATION_SOURCE).execute_test().command_is_successful()
AwsTest('Make replication destination').create_bucket(REPLICATION_DEST).execute_test().command_is_successful()

AwsTest('Get nonexistent replication').get_bucket_replication(REPLICATION_SOURCE).execute_test(negative_case=True).command_should_fail()
AwsTest('Delete nonexistent replication').delete_bucket_replication(REPLICATION_SOURCE).execute_test().command_is_successful()

put_replication(REPLICATION_SOURCE, REPLICATION_POLICY)
AwsTest('Get replication').get_bucket_replication(REPLICATION_SOURCE).execute_test().command_is_successful()
AwsTest('Delete replication').delete_bucket_replication(REPLICATION_SOURCE).execute_test().command_is_successful()

put_replication(REPLICATION_SOURCE, REPLICATION_POLICY)
rep_test = AwsTest('Get replication').get_bucket_replication(REPLICATION_SOURCE).execute_test().command_is_successful()
rep_policy = json.loads(rep_test.status.stdout)
# Known issue: the old v1 format gets translated to v2,
# so it doesn't match the provided policy verbatim
# (though it's functionally equivalent). Currently just
# checking that we got valid JSON, should check that the
# policy is the same as what we gave once v2 format is
# available in the test environment.

AwsTest('Put object: wrong prefix').put_object(REPLICATION_SOURCE, 'no_replication', filesize=1024, key_name='bad1').execute_test().command_is_successful()
rep_test = AwsTest('Check replication status').head_object(REPLICATION_SOURCE, 'bad1').execute_test().command_is_successful()
if 'ReplicationStatus' in json.loads(rep_test.status.stdout):
    raise AssertionError("Unexpected replication status")

AwsTest('Put object: matching prefix').put_object(REPLICATION_SOURCE, 'test_replication', filesize=1024, key_name='test1').execute_test().command_is_successful()
rep_test = AwsTest('Check replication status').head_object(REPLICATION_SOURCE, 'test1').execute_test().command_is_successful()
if not 'ReplicationStatus' in json.loads(rep_test.status.stdout):
    raise AssertionError("Unexpected replication status")

#******* Delete bucket **********
AwsTest('Aws can delete bucket').delete_bucket("seagatebuckettag").execute_test().command_is_successful()

#************ Authorize copy-object ********************

load_test_config()

#Create account sourceAccount

test_msg = "Create account sourceAccount"
source_account_args = {'AccountName': 'sourceAccount', 'Email': 'sourceAccount@seagate.com', \
                   'ldapuser': S3ClientConfig.ldapuser, \
                   'ldappasswd': S3ClientConfig.ldappasswd}
account_response_pattern = "AccountId = [\w-]*, CanonicalId = [\w-]*, RootUserName = [\w+=,.@-]*, AccessKeyId = [\w-]*, SecretKey = [\w/+]*$"
result1 = AuthTest(test_msg).create_account(**source_account_args).execute_test()
result1.command_should_match_pattern(account_response_pattern)
account_response_elements = get_response_elements(result1.status.stdout)
source_access_key_args = {}
source_access_key_args['AccountName'] = "sourceAccount"
source_access_key_args['AccessKeyId'] = account_response_elements['AccessKeyId']
source_access_key_args['SecretAccessKey'] = account_response_elements['SecretKey']
#Create account destinationAccount

test_msg = "Create account destinationAccount"
target_account_args = {'AccountName': 'destinationAccount', 'Email': 'destinationAccount@seagate.com', \
                'ldapuser': S3ClientConfig.ldapuser, \
                'ldappasswd': S3ClientConfig.ldappasswd}
account_response_pattern = "AccountId = [\w-]*, CanonicalId = [\w-]*, RootUserName = [\w+=,.@-]*, AccessKeyId = [\w-]*, SecretKey = [\w/+]*$"
result1 = AuthTest(test_msg).create_account(**target_account_args).execute_test()
result1.command_should_match_pattern(account_response_pattern)
account_response_elements = get_response_elements(result1.status.stdout)
destination_access_key_args = {}
destination_access_key_args['AccountName'] = "destinationAccount"
destination_access_key_args['AccessKeyId'] = account_response_elements['AccessKeyId']
destination_access_key_args['SecretAccessKey'] = account_response_elements['SecretKey']
#Create account crossAccount

test_msg = "Create account crossAccount"
cross_account_args = {'AccountName': 'crossAccount', 'Email': 'crossAccount@seagate.com', \
                'ldapuser': S3ClientConfig.ldapuser, \
                'ldappasswd': S3ClientConfig.ldappasswd}
account_response_pattern = "AccountId = [\w-]*, CanonicalId = [\w-]*, RootUserName = [\w+=,.@-]*, AccessKeyId = [\w-]*, SecretKey = [\w/+]*$"
result1 = AuthTest(test_msg).create_account(**cross_account_args).execute_test()
result1.command_should_match_pattern(account_response_pattern)
account_response_elements = get_response_elements(result1.status.stdout)
cross_access_key_args = {}
cross_access_key_args['AccountName'] = "crossAccount"
cross_access_key_args['AccessKeyId'] = account_response_elements['AccessKeyId']
cross_access_key_args['SecretAccessKey'] = account_response_elements['SecretKey']
cross_access_key_args['CanonicalId'] = account_response_elements['CanonicalId']

#------------------------create source bucket---------------------------
os.environ["AWS_ACCESS_KEY_ID"] = source_access_key_args['AccessKeyId']
os.environ["AWS_SECRET_ACCESS_KEY"] = source_access_key_args['SecretAccessKey']

AwsTest('Aws can create bucket').create_bucket("source-bucket").execute_test().command_is_successful()
AwsTest('Aws can create object').put_object("source-bucket", "source-object", 93323264)\
    .execute_test().command_is_successful()
#--------------------------create target bucket----------------------
os.environ["AWS_ACCESS_KEY_ID"] = destination_access_key_args['AccessKeyId']
os.environ["AWS_SECRET_ACCESS_KEY"] = destination_access_key_args['SecretAccessKey']

AwsTest('Aws can create bucket').create_bucket("target-bucket").execute_test().command_is_successful()

#---------------------------copy-object------------------------------
os.environ["AWS_ACCESS_KEY_ID"] = cross_access_key_args['AccessKeyId']
os.environ["AWS_SECRET_ACCESS_KEY"] = cross_access_key_args['SecretAccessKey']
AwsTest('Aws cannot copy object').copy_object("source-bucket/source-object", "target-bucket", "source-object")\
    .execute_test(negative_case=True).command_should_fail().command_error_should_have("AccessDenied")
os.environ["AWS_ACCESS_KEY_ID"] = source_access_key_args['AccessKeyId']
os.environ["AWS_SECRET_ACCESS_KEY"] = source_access_key_args['SecretAccessKey']
policy_source_bucket_relative = os.path.join(os.path.dirname(__file__), 'policy_files', 'source_bucket.json')
policy_source_bucket = "file://" + os.path.abspath(policy_source_bucket_relative)
AwsTest("Aws can put policy on bucket").put_bucket_policy("source-bucket", policy_source_bucket).execute_test().command_is_successful()
os.environ["AWS_ACCESS_KEY_ID"] = cross_access_key_args['AccessKeyId']
os.environ["AWS_SECRET_ACCESS_KEY"] = cross_access_key_args['SecretAccessKey']
AwsTest('Aws can not copy object').copy_object("source-bucket/source-object", "target-bucket", "source-object")\
    .execute_test(negative_case=True).command_should_fail().command_error_should_have("AccessDenied")
os.environ["AWS_ACCESS_KEY_ID"] = destination_access_key_args['AccessKeyId']
os.environ["AWS_SECRET_ACCESS_KEY"] = destination_access_key_args['SecretAccessKey']
policy_target_bucket_relative = os.path.join(os.path.dirname(__file__), 'policy_files', 'target_bucket.json')
policy_target_bucket = "file://" + os.path.abspath(policy_target_bucket_relative)
AwsTest("Aws can put policy on bucket").put_bucket_policy("target-bucket", policy_target_bucket).execute_test().command_is_successful()
os.environ["AWS_ACCESS_KEY_ID"] = cross_access_key_args['AccessKeyId']
os.environ["AWS_SECRET_ACCESS_KEY"] = cross_access_key_args['SecretAccessKey']
AwsTest('Aws can copy object').copy_object("source-bucket/source-object", "target-bucket", "source-object")\
    .execute_test().command_is_successful()


# Put object tags on source object
os.environ["AWS_ACCESS_KEY_ID"] = source_access_key_args['AccessKeyId']
os.environ["AWS_SECRET_ACCESS_KEY"] = source_access_key_args['SecretAccessKey']
AwsTest('Aws can put tags on source object')\
    .put_object_tagging("source-bucket", "source-object", [{'Key': 'seagate','Value': 'storage'}] )\
        .execute_test().command_is_successful()

# Copy Object should fail now
AwsTest('Aws can not copy source object with tags')\
    .copy_object("source-bucket/source-object", "target-bucket", "copy1")\
    .execute_test(negative_case=True).command_should_fail().command_error_should_have("AccessDenied")

# Update bucket policy on destination bucket to allow PutObjectTagging
policy_target_bucket_relative_new = os.path.join(os.path.dirname(__file__), 'policy_files', 'target_bucket_putobjecttagging.json')
policy_target_bucket_new = "file://" + os.path.abspath(policy_target_bucket_relative_new)

os.environ["AWS_ACCESS_KEY_ID"] = destination_access_key_args['AccessKeyId']
os.environ["AWS_SECRET_ACCESS_KEY"] = destination_access_key_args['SecretAccessKey']

AwsTest("Aws can update bucket policy on target bucket")\
    .put_bucket_policy("target-bucket", policy_target_bucket_new)\
    .execute_test().command_is_successful()

# Copy object should pass now
os.environ["AWS_ACCESS_KEY_ID"] = source_access_key_args['AccessKeyId']
os.environ["AWS_SECRET_ACCESS_KEY"] = source_access_key_args['SecretAccessKey']
AwsTest('Aws can copy object after policy update').copy_object("source-bucket/source-object", "target-bucket", "copy1")\
    .execute_test().command_is_successful()

# update destination bucket policy to deny putobjectacl permission
policy_target_bucket_relative_acl = os.path.join(os.path.dirname(__file__), 'policy_files', 'target_bucket_denyobjectacl.json')
policy_target_bucket_acl = "file://" + os.path.abspath(policy_target_bucket_relative_acl)

os.environ["AWS_ACCESS_KEY_ID"] = destination_access_key_args['AccessKeyId']
os.environ["AWS_SECRET_ACCESS_KEY"] = destination_access_key_args['SecretAccessKey']

AwsTest("Aws can update bucket policy on target bucket to deny putobjectacl")\
    .put_bucket_policy("target-bucket", policy_target_bucket_acl)\
    .execute_test().command_is_successful()

# Copy object should fail with acl header
os.environ["AWS_ACCESS_KEY_ID"] = source_access_key_args['AccessKeyId']
os.environ["AWS_SECRET_ACCESS_KEY"] = source_access_key_args['SecretAccessKey']
AwsTest('Aws cannot copy object after policy update')\
    .copy_object("source-bucket/source-object", "target-bucket", "copy2", "bucket-owner-full-control")\
        .execute_test(negative_case=True).command_should_fail().command_error_should_have("AccessDenied")

os.environ["AWS_ACCESS_KEY_ID"] = source_access_key_args['AccessKeyId']
os.environ["AWS_SECRET_ACCESS_KEY"] = source_access_key_args['SecretAccessKey']
AwsTest("Aws can delete policy on bucket").delete_bucket_policy("source-bucket").execute_test().command_is_successful()
os.environ["AWS_ACCESS_KEY_ID"] = destination_access_key_args['AccessKeyId']
os.environ["AWS_SECRET_ACCESS_KEY"] = destination_access_key_args['SecretAccessKey']
# Copy Object should fail now
AwsTest('Aws can not copy source object with tags but without GetTagging permission')\
    .copy_object("source-bucket/source-object", "target-bucket", "copy1")\
    .execute_test(negative_case=True).command_should_fail().command_error_should_have("AccessDenied")

AwsTest("Aws can delete policy on bucket").delete_bucket_policy("target-bucket").execute_test().command_is_successful()
AwsTest('Aws can delete object').delete_object("target-bucket", "source-object")\
    .execute_test().command_is_successful()
AwsTest('Aws can delete object').delete_object("target-bucket", "copy1")\
    .execute_test().command_is_successful()

os.environ["AWS_ACCESS_KEY_ID"] = source_access_key_args['AccessKeyId']
os.environ["AWS_SECRET_ACCESS_KEY"] = source_access_key_args['SecretAccessKey']

AwsTest('Owner account can do put object acl').put_object_acl("source-bucket", "source-object", "grant-full-control" ,"id=" + cross_access_key_args['CanonicalId'] )\
.execute_test().command_is_successful()
os.environ["AWS_ACCESS_KEY_ID"] = cross_access_key_args['AccessKeyId']
os.environ["AWS_SECRET_ACCESS_KEY"] = cross_access_key_args['SecretAccessKey']
AwsTest('Aws can not copy object').copy_object("source-bucket/source-object", "target-bucket", "source-object")\
    .execute_test(negative_case=True).command_should_fail().command_error_should_have("AccessDenied")
os.environ["AWS_ACCESS_KEY_ID"] = destination_access_key_args['AccessKeyId']
os.environ["AWS_SECRET_ACCESS_KEY"] = destination_access_key_args['SecretAccessKey']

AwsTest('Aws can put bucket acl').put_bucket_acl("target-bucket", "grant-write", "id=" + cross_access_key_args['CanonicalId'] ).execute_test().command_is_successful()
os.environ["AWS_ACCESS_KEY_ID"] = cross_access_key_args['AccessKeyId']
os.environ["AWS_SECRET_ACCESS_KEY"] = cross_access_key_args['SecretAccessKey']
AwsTest('Aws can copy object').copy_object("source-bucket/source-object", "target-bucket", "source-object")\
    .execute_test().command_is_successful()


os.environ["AWS_ACCESS_KEY_ID"] = source_access_key_args['AccessKeyId']
os.environ["AWS_SECRET_ACCESS_KEY"] = source_access_key_args['SecretAccessKey']

AwsTest('Aws can delete object').delete_object("source-bucket", "source-object")\
    .execute_test().command_is_successful()
AwsTest('Aws can delete bucket').delete_bucket("source-bucket")\
    .execute_test().command_is_successful()

os.environ["AWS_ACCESS_KEY_ID"] = cross_access_key_args['AccessKeyId']
os.environ["AWS_SECRET_ACCESS_KEY"] = cross_access_key_args['SecretAccessKey']

AwsTest('Aws can delete object').delete_object("target-bucket", "source-object")\
    .execute_test().command_is_successful()

AwsTest('Aws can delete bucket').delete_bucket("target-bucket")\
    .execute_test().command_is_successful()

#-------------------Delete Accounts------------------
test_msg = "Delete account sourceAccount"
s3test_access_key = S3ClientConfig.access_key_id
s3test_secret_key = S3ClientConfig.secret_key
S3ClientConfig.access_key_id = source_access_key_args['AccessKeyId']
S3ClientConfig.secret_key = source_access_key_args['SecretAccessKey']

account_args = {'AccountName': 'sourceAccount', 'Email': 'sourceAccount@seagate.com',  'force': True}
AuthTest(test_msg).delete_account(**source_account_args).execute_test()\
            .command_response_should_have("Account deleted successfully")

test_msg = "Delete account destinationAccount"
S3ClientConfig.access_key_id = destination_access_key_args['AccessKeyId']
S3ClientConfig.secret_key = destination_access_key_args['SecretAccessKey']
account_args = {'AccountName': 'destinationAccount', 'Email': 'destinationAccount@seagate.com',  'force': True}
AuthTest(test_msg).delete_account(**target_account_args).execute_test()\
            .command_response_should_have("Account deleted successfully")

test_msg = "Delete account crossAccount"
account_args = {'AccountName': 'crossAccount', 'Email': 'crossAccount@seagate.com',  'force': True}
S3ClientConfig.access_key_id = cross_access_key_args['AccessKeyId']
S3ClientConfig.secret_key = cross_access_key_args['SecretAccessKey']
AuthTest(test_msg).delete_account(**cross_account_args).execute_test()\
            .command_response_should_have("Account deleted successfully")
S3ClientConfig.access_key_id = s3test_access_key
S3ClientConfig.secret_key = s3test_secret_key
