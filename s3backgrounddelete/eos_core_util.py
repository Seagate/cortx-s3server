import string
import hmac
import hashlib
from hashlib import sha1, sha256
import urllib
import datetime
from eos_core_config import EOSCoreConfig


def create_canonical_request(
        method, canonical_uri, canonical_query_string, body, epoch_t, host):

    signed_headers = 'host;x-amz-date'
    payload_hash = hashlib.sha256(body.encode('utf-8')).hexdigest()
    canonical_headers = 'host:' + host + '\n' + \
        'x-amz-date:' + get_timestamp(epoch_t) + '\n'
    canonical_request = method + '\n' + canonical_uri + '\n' + canonical_query_string + '\n' + \
        canonical_headers + '\n' + signed_headers + '\n' + payload_hash
    return canonical_request


def sign(key, msg):
    return hmac.new(key, msg.encode('utf-8'), hashlib.sha256).digest()


def getV4SignatureKey(key, dateStamp, regionName, serviceName):
    kDate = sign(('AWS4' + key).encode('utf-8'), dateStamp)
    kRegion = sign(kDate, regionName)
    kService = sign(kRegion, serviceName)
    kSigning = sign(kService, 'aws4_request')

    return kSigning


def create_string_to_sign_v4(method='', canonical_uri='', canonical_query_string='', body='', epoch_t='',
                             algorithm='', host='', service='', region=''):

    canonical_request = create_canonical_request(method, canonical_uri, canonical_query_string,
                                                 body, epoch_t, host)

    credential_scope = get_date(epoch_t) + '/' + \
        region + '/' + service + '/' + 'aws4_request'

    string_to_sign = algorithm + '\n' + get_timestamp(epoch_t) + '\n' + credential_scope \
        + '\n' + hashlib.sha256(canonical_request.encode('utf-8')).hexdigest()
    return string_to_sign


def sign_request_v4(method=None, canonical_uri='/', canonical_query_string='', body='', epoch_t='',
                    host='', service='', region=''):

    if method is None:
        print("method can not be null")
        return None
    credential_scope = get_date(epoch_t) + '/' + region + \
        '/' + service + '/' + 'aws4_request'

    signed_headers = 'host;x-amz-date'

    algorithm = 'AWS4-HMAC-SHA256'

    config = EOSCoreConfig()
    access_key = config.get_eos_core_access_key()
    secret_key = config.get_eos_core_secret_key()

    string_to_sign = create_string_to_sign_v4(method, canonical_uri, canonical_query_string, body, epoch_t,
                                              algorithm, host, service, region)

    signing_key = getV4SignatureKey(
        secret_key, get_date(epoch_t), region, service)

    signature = hmac.new(
        signing_key,
        (string_to_sign).encode('utf-8'),
        hashlib.sha256).hexdigest()

    authorization_header = algorithm + ' ' + 'Credential=' + access_key + '/' + \
        credential_scope + ', ' + 'SignedHeaders=' + signed_headers + \
        ', ' + 'Signature=' + signature
    return authorization_header


def get_date(epoch_t):
    return epoch_t.strftime('%Y%m%d')


def get_timestamp(epoch_t):
    return epoch_t.strftime('%Y%m%dT%H%M%SZ')


def prepare_signed_header(http_request, request_uri, query_params, body):
    config = EOSCoreConfig()
    url_parse_result = urllib.parse.urlparse(config.get_eos_core_endpoint())
    epoch_t = datetime.datetime.utcnow()
    headers = {'content-type': 'application/x-www-form-urlencoded',
               'Accept': 'text/plain'}
    headers['Authorization'] = sign_request_v4(http_request, request_uri, query_params, body, epoch_t, url_parse_result.netloc,
                                               config.get_eos_core_service(), config.get_eos_core_region())
    headers['X-Amz-Date'] = get_timestamp(epoch_t)
    return headers
