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
import sys
import time
from threading import Timer
import subprocess
from framework import PyCliTest
from framework import Config
from framework import logit
from s3client_config import S3ClientConfig

class S3fiTest(PyCliTest):
    def __init__(self, description):
        self.s3cfg = os.path.join(os.path.dirname(os.path.realpath(__file__)), Config.config_file)
        super(S3fiTest, self).__init__(description)

    def setup(self):
        # Do initializations required to run the tests
        logit("Setting up the test [%s]" % (self.description))
        super(S3fiTest, self).setup()

    def run(self):
        super(S3fiTest, self).run()

    def teardown(self):
        super(S3fiTest, self).teardown()

    def with_cli(self, command):
        if Config.no_ssl:
            command = command + S3ClientConfig.s3_uri_http
        else:
            command = command + S3ClientConfig.s3_uri_https + " --cacert " + S3ClientConfig.ca_file
        super(S3fiTest, self).with_cli(command)

    def enable_fi(self, opcode, freq, tag):
        curl_cmd = "curl -sS --header \"x-seagate-faultinjection: "
        self.opcode = opcode
        self.freq = freq
        self.tag = tag
        command = curl_cmd + self.opcode + "," + self.freq + "," + self.tag + "\" " + "-X PUT "
        self.with_cli(command)
        return self

    def enable_fi_random(self, opcode, tag, prob):
        curl_cmd = "curl -sS --header \"x-seagate-faultinjection: "
        self.opcode = opcode
        self.tag = tag
        self.prob = prob
        command = curl_cmd + self.opcode + ",random," + self.tag +  "," + self.prob + "\" " + "-X PUT "
        self.with_cli(command)
        return self

    def enable_fi_enablen(self, opcode, tag, ntime):
        curl_cmd = "curl -sS --header \"x-seagate-faultinjection: "
        self.opcode = opcode
        self.tag = tag
        self.ntime = ntime
        command = curl_cmd + self.opcode + ",enablen," + self.tag +  "," + self.ntime + "\" " + "-X PUT "
        self.with_cli(command)
        return self

    def enable_fi_offnonm(self, opcode, tag, ntime, mtime):
        # sleep to avoid the impact on previous request cleanup of fault injection
        # TODO fault injection should be embeded into actual request.
        # This will restrict the fault injection scope/lifetime to that specific request only.
        time.sleep(1)
        curl_cmd = "curl -sS --header \"x-seagate-faultinjection: "
        self.opcode = opcode
        self.tag = tag
        self.ntime = ntime
        self.mtime = mtime
        command = curl_cmd + self.opcode + ",offnonm," + self.tag +  "," + self.ntime +  "," + self.mtime + "\" " + "-X PUT "
        self.with_cli(command)
        return self

    def disable_fi(self, tag):
        curl_cmd = "curl -sS --header \"x-seagate-faultinjection: "
        self.tag = tag
        command = curl_cmd + "disable,noop," + self.tag + "\" " + "-X PUT "
        self.with_cli(command)
        return self
