
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

    def enable_fi(self, opcode, freq, tag):
        curl_cmd = "curl -sS --header \"x-seagate-faultinjection: "
        self.opcode = opcode
        self.freq = freq
        self.tag = tag
        command = curl_cmd + self.opcode + "," + self.freq + "," + self.tag + "\" " + "-X PUT " + S3ClientConfig.s3_uri
        self.with_cli(command)
        return self

    def enable_fi_random(self, opcode, tag, prob):
        curl_cmd = "curl -sS --header \"x-seagate-faultinjection: "
        self.opcode = opcode
        self.tag = tag
        self.prob = prob
        command = curl_cmd + self.opcode + ",random, " + self.tag +  "," + self.prob + "\ " + "-X PUT " + S3ClientConfig.s3_uri
        self.with_cli(command)
        return self

    def enable_fi_enablen(self, opcode, tag, ntime):
        curl_cmd = "curl -sS --header \"x-seagate-faultinjection: "
        self.opcode = opcode
        self.tag = tag
        self.ntime = ntime
        command = curl_cmd + self.opcode + ",enablen, " + self.tag +  "," + self.ntime + "\ " + "-X PUT " + S3ClientConfig.s3_uri
        self.with_cli(command)
        return self

    def enable_fi_offnonm(self, opcode, tag, ntime, mtime):
        curl_cmd = "curl -sS --header \"x-seagate-faultinjection: "
        self.opcode = opcode
        self.tag = tag
        self.ntime = ntime
        self.mtime = mtime
        command = curl_cmd + self.opcode + ",offnonm, " + self.tag +  "," + self.ntime +  "," + self.mtime + "\ " + "-X PUT " + S3ClientConfig.s3_uri
        self.with_cli(command)
        return self

    def disable_fi(self, tag):
        curl_cmd = "curl -sS --header \"x-seagate-faultinjection: "
        self.tag = tag
        command = curl_cmd + "disable,noop," + self.tag + "\" " + "-X PUT " + S3ClientConfig.s3_uri
        self.with_cli(command)
        return self
