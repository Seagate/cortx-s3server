import os
import yaml
import pprint
import http.client, urllib.parse
import ssl
import json
import xmltodict
import re
from framework import Config
from s3client_config import S3ClientConfig

class AuthHTTPClient:
    def __init__(self):
        self.iam_uri_https = S3ClientConfig.iam_uri_https
        self.iam_uri_http = S3ClientConfig.iam_uri_http

    def authenticate_user(self, headers, body):

        if Config.no_ssl:
            conn = http.client.HTTPConnection(urllib.parse.urlparse(self.iam_uri_http).netloc)
        else:
            conn = http.client.HTTPSConnection(urllib.parse.urlparse(self.iam_uri_https).netloc,
                                           context= ssl._create_unverified_context())
        conn.request("POST", "/", urllib.parse.urlencode(body), headers)
        response_data = conn.getresponse().read()
        conn.close()
        return response_data

def check_response(expected_response, test_response):
    #assert test_response.decode("utf-8") in expected_response
    #Request id is dynamically generated, so compare string
    #skipping request id
    expected = expected_response.split("0000")
    assert expected[0] in test_response.decode("utf-8")
    assert expected[1] in test_response.decode("utf-8")
    print("Response has [%s]." % (test_response))

test_data = {}
test_data_file = os.path.join(os.path.dirname(__file__), 'auth_spec_signcalc_test_data.yaml')
with open(test_data_file, 'r') as f:
    test_data = yaml.safe_load(f)

for test in test_data:
    print("Test case [%s] - " % test_data[test]['test-title'])
    headers = test_data[test]['req-headers']
    params = test_data[test]['req-params']
    expected_response = test_data[test]['output']
    test_response = AuthHTTPClient().authenticate_user(headers, params)
    check_response(expected_response, test_response)
    print("Test was successful\n")
