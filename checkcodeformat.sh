#!/bin/bash
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

# Script to check whether coding style complies with clang-format.
#   Note: Only C/C++/Header files are checked for coding style.

# For modified unstaged, staged and commited files:
#   Dry-run 'git clang-format' and check whether code changes compy
#   with 'clang-format'.
clang_failed=0
git clang-format --style=Google --extensions=c,cc,h,java --diff --commit HEAD~1 \
    | grep -E 'clang-format did not modify any files|no modified files to format' \
    > /dev/null
if [ $? -ne 0 ]
then
  clang_failed=1
  printf "%s\n%s\n%s\n\t%s\n\n" \
          "[ERROR:Code formatting]" \
          "One or more modified files do not comply with clang-format." \
          "Run below command to see required changes:" \
          "'git clang-format --style=Google --extensions=c,cc,h,java --diff --commit HEAD~1'"
fi

# For untracked files (new files):
#   Find all untracked *.cc/*.c/*.h/*.java files under s3server dir.
#   And check whether new files comply with 'clang-format'.
tmpfile=/tmp/s3codeformat.$(date "+%Y.%m.%d-%H.%M.%S")
msg_printed=0
while read filename
do
  clang-format -style=Google $filename > $tmpfile
  diff $filename $tmpfile > /dev/null
  if [ $? -ne 0 ]
  then
    if [ $msg_printed -ne 1 ]
    then
      printf "%s\n%s\n%s\n\t%s\n\n" \
              "[ERROR:Code formatting]" \
              "Below newly added files do not comply with clang-format." \
              "To format the code use:" \
              "'clang-format -style=Google -i <filename>'"
      msg_printed=1
    fi
    printf "\t%s\n" "$filename"
    clang_failed=1
  fi
done < <(git status -uall -s server/ ut/ | grep '??' | grep  -E '\.(c|cc|h|java)$' \
        | awk '{print $2}')

rm -f $tmpfile

if [ $clang_failed -ne 0 ]
then
  printf "\n%s\n" "Code formatting check...Failed"
  exit 1
fi

printf "%s\n" "Code formatting check...Okay"
exit 0
