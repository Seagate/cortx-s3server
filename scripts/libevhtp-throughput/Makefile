#!/usr/bin/make -f
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

THIS_FILE := $(lastword $(MAKEFILE_LIST))
THIS_DIR  := $(abspath $(dir $(THIS_FILE)))
BUILD_ROOT_DIR := $(abspath $(THIS_DIR)/../..)

SERVER := $(THIS_DIR)/server
CLIENT := $(THIS_DIR)/client

.PHONY: all

all: $(SERVER) $(CLIENT)

$(SERVER): $(THIS_DIR)/server.cpp
	g++ -o $@ --std=c++11 $< \
	-I $(BUILD_ROOT_DIR)/third_party/libevent/s3_dist/include \
	-I $(BUILD_ROOT_DIR)/third_party/libevhtp/s3_dist/include/evhtp \
	-L $(BUILD_ROOT_DIR)/third_party/libevent/s3_dist/lib \
	-L $(BUILD_ROOT_DIR)/third_party/libevhtp/s3_dist/lib \
	-lgflags -levhtp -levent -levent_pthreads -levent_openssl -lssl -lcrypto \
	-pthread -Wl,-rpath,/opt/seagate/cortx/s3/libevent

$(CLIENT): $(THIS_DIR)/client.cpp
	g++ -o $@ --std=c++11 -lgflags -lcurl $<

