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

#!/bin/sh -xe
# Script to setup jsoncpp.
# git repo: https://github.com/open-source-parsers/jsoncpp.git
# tag: 1.6.5 commit: d84702c9036505d0906b2f3d222ce5498ec65bc6

cd jsoncpp

# Generate amalgamated source for inclusion in project.
# https://github.com/open-source-parsers/jsoncpp#using-jsoncpp-in-your-project
python amalgamate.py

cd ..
