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
from datetime import datetime

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


def update_date_headers(params):
    if 'Authorization' in params :
        origional_headers = params['Authorization']
        authorization_headers = origional_headers.split()
        # Handle aws v4 signature
        if 'AWS4-HMAC-SHA256' in authorization_headers:
            request_scope = authorization_headers[1]
            credentaial_scopes = request_scope.split("/")
            credentaial_scopes[1] = get_canonical_scope_date()
            params['Authorization'] = authorization_headers[0] + " " + credentaial_scopes[0] + "/" + get_canonical_scope_date() + "/" + credentaial_scopes[2]+"/"+credentaial_scopes[3]+"/"+credentaial_scopes[4]+" "+ authorization_headers[2]+" "+ authorization_headers[3]
            if 'X-Amz-Date' in params:
               params['X-Amz-Date'] = get_current_timestamp_v4_request()
            elif 'Date' in params:
               params['Date'] = get_current_timestamp_v4_request()
        # Handle aws v2 signature
        elif 'AWS' in authorization_headers:
         if 'X-Amz-Date' in params:
            params['X-Amz-Date'] = get_current_timestamp_v2_request()
         elif 'Date' in params:
            params['Date'] = get_current_timestamp_v2_request()
    return params


# Get current date in 'yyyyMMdd'T'hhmmssZ' e.g. 20191219T102148Z
def get_current_timestamp_v4_request():
    epoch_t = datetime.utcnow()
    return epoch_t.strftime('%Y%m%dT%H%M%SZ')

# Get current date in 'yyyyMMdd' e.g. 20191219
def get_canonical_scope_date():
    epoch_t = datetime.utcnow()
    return epoch_t.strftime('%Y%m%d')

# Get current date in GMT format e.g. Thu, 05 Jul 2018 03:55:43 GMT
def get_current_timestamp_v2_request():
    epoch_t = datetime.now()
    return epoch_t.strftime('%a, %d %b %Y %I:%M:%S %ZGMT')


test_data = {}
test_data_file = os.path.join(os.path.dirname(__file__), 'auth_spec_signcalc_test_data.yaml')
with open(test_data_file, 'r') as f:
    test_data = yaml.safe_load(f)
'''
TODO handle signature calculation for each request
for test in test_data:
    print("Test case [%s] - " % test_data[test]['test-title'])
    headers = test_data[test]['req-headers']
    params = test_data[test]['req-params']
    params = update_date_headers(params)
    expected_response = test_data[test]['output']
    print(params)
    test_response = AuthHTTPClient().authenticate_user(headers, params)
    print(test_response)
    check_response(expected_response, test_response)
    print("Test was successful\n")
'''
