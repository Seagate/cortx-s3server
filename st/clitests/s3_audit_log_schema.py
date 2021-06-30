import os
import yaml
import json
from framework import Config
from awss3api import AwsTest

#Config.log_enabled = True


cmd = "curl -s -X GET -H \"x-seagate-mgmt-api: true\" -H \"Connection: close\" http://s3.seagate.com/s3/audit-log/schema"
AwsTest('S3_audit_log schema').execute_curl(cmd).\
execute_test().command_is_successful().command_response_should_have("\"field_id\": \"authentication_type\"")
