#!/bin/bash
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


#install pylint
echo "installing pylint...."
yum -y install pylint > /dev/null

#check if pylint is installed or not
if [ "rpm -q pylint" ]
then
   version=$(rpm -q --qf "%{Version}" pylint)
   echo "pylint $version is installed successfully"
else
  echo "pylint installation failed"
fi

echo "installing autopep8...."
#install autopep8 :automatically formats python code
#to conform to the PEP 8 style guide.
pip3 install --upgrade autopep8 2> /dev/null

#check if autopep8 is installed or not
if [ "pip3 list | grep autopep8" ]
then
   autopep8version=$(pip3 show autopep8 | grep Version)
   echo "autopep8 $autopep8version is installed successfully"
else
  echo "autopep8 installation failed"
fi

