#!/bin/sh
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

# Script to generate potential object leak due to parallel/concurrent PUT requests

aws s3 rb s3://mybucket --force
rm -rf ./paralleltest/*
echo Creating new bucket "mybucket"
aws s3 mb s3://mybucket
filecnt=12
paralel=$((filecnt / 4))
mkdir -p ./paralleltest
# Create $filecnt files, each of 1MB size
for value in $(seq -f "file%.0f" $filecnt)
do
dd if=/dev/urandom of=./paralleltest/$value bs=1M count=1 iflag=fullblock
done
echo "Created $filecnt files"

# Write ${filecnt} files/objects to bucket 'mybucket', in parallel ($paralel at a time)
seq -f "file%.0f" $filecnt| xargs -n 1 -t -P $paralel -I {} aws s3 cp `pwd`/paralleltest/{} s3://mybucket/POCKey

# Cleanup
rm -rf ./paralleltest/*
rm -rf ./paralleltest
