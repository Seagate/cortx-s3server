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

#!/bin/sh -x

rm -rf cpp-driver

git clone --branch 2.1.0 --depth=1 https://github.com/datastax/cpp-driver.git
cd cpp-driver

INSTALL_DIR=`pwd`/s3_dist/usr/

mkdir -p $INSTALL_DIR
mkdir build
cd build

cmake -DCMAKE_INSTALL_PREFIX:PATH=$INSTALL_DIR ..

make
make install
cd ..

# make necessary adjustments for packaging.
fpm -s dir -t rpm -C s3_dist/ --name libcassandra --version 2.1.0 --description "DataStax C/C++ Driver for Apache Cassandra"

cd ..
