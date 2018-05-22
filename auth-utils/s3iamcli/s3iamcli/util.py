import sys
import string
import hmac
import base64
from hashlib import sha1, sha256
from s3iamcli.config import Credentials

def utf8_encode(msg):
    return msg.encode('UTF-8')

def utf8_decode(msg):
    return str(msg, 'UTF-8')

# if x-amz-* has multiple values then value for that header should be passed as
# list of values eg. headers['x-amz-authors'] = ['Jack', 'Jelly']
# example return value: x-amz-authors:Jack,Jelly\nx-amz-org:Seagate\n
def _get_canonicalized_xamz_headers(headers):
    xamz_headers = ''
    for header in sorted(headers.keys()):
        if header.startswith("x-amz-"):
            if type(headers[header]) is str:
                xamz_headers += header + ":" + headers[header] + "\n"
            elif type(headers[header]) is list:
                xamz_headers += header + ":" + ','.join(headers[header]) + "\n"

    return xamz_headers

def _get_canonicalized_resource(canonical_uri, params):
    # TODO: if required
    pass


def _create_str_to_sign(http_method, canonical_uri, params, headers):
    str_to_sign = http_method + '\n'
    str_to_sign += headers.get("content-md5", "") + "\n"
    str_to_sign += headers.get("content-type", "") + "\n"
    str_to_sign += headers.get("date", "") + "\n"
    str_to_sign += _get_canonicalized_xamz_headers(headers)

    # canonicalized_resource = _get_canonicalized_resource(canonical_uri, params)
    # replace canonical_uri with canonicalized_resource once
    # _get_canonicalized_resource() is implemented
    str_to_sign += canonical_uri

    str_to_sign = utf8_encode(str_to_sign)

    return str_to_sign

def sign_request_v2(method='GET', canonical_uri='/', params={}, headers={}):
    access_key = Credentials.access_key
    secret_key = Credentials.secret_key

    str_to_sign = _create_str_to_sign(method, canonical_uri, params, headers)

    signature = utf8_decode(base64.encodestring(
        hmac.new(utf8_encode(secret_key), str_to_sign, sha1).digest()).strip())

    auth_header = "AWS %s:%s" % (access_key, signature)

    return auth_header
