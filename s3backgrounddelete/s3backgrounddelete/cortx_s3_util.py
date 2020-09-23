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

"""This is utility class used for Authorization."""
import string
import hmac
import hashlib
from hashlib import sha1, sha256
import urllib
import datetime
from s3backgrounddelete.cortx_s3_config import CORTXS3Config

class CORTXS3Util(object):
   """Generate Authorization headers to validate requests."""
   _config = None

   def __init__(self, config):
        """Initialise config."""
        if (config is None):
            self._config = CORTXS3Config()
        else:
            self._config = config

   def get_headers(self, host, epoch_t, body_256hash):
        headers = {
           'host': host,
           'x-amz-content-sha256': body_256hash,
           'x-amz-date': self.get_amz_timestamp(epoch_t)
        }
        return headers

   def create_canonical_request(self, method, canonical_uri, canonical_query_string, body, epoch_t, host):
       """Create canonical request based on uri and query string."""

       body_256sha_hex =  hashlib.sha256(body.encode('utf-8')).hexdigest()

       self.body_hash_hex = body_256sha_hex
       headers = self.get_headers(host, epoch_t, body_256sha_hex)
       sorted_headers = sorted([k for k in headers])
       canonical_headers = ""
       for key in sorted_headers:
           canonical_headers += "{}:{}\n".format(key.lower(), headers[key].strip())

       signed_headers = "{}".format(";".join(sorted_headers))
       canonical_request = method + '\n' + canonical_uri + '\n' + canonical_query_string + '\n' + \
           canonical_headers + '\n' + signed_headers + '\n' + body_256sha_hex
       return canonical_request

   def sign(self, key, msg):
       """Return hmac value based on key and msg."""
       return hmac.new(key, msg.encode('utf-8'), hashlib.sha256).digest()

   def getV4SignatureKey(self, key, dateStamp, regionName, serviceName):
       """Generate v4SignatureKey based on key, datestamp, region and service name."""
       kDate = self.sign(('AWS4' + key).encode('utf-8'), dateStamp)
       kRegion = self.sign(kDate, regionName)
       kService = self.sign(kRegion, serviceName)
       kSigning = self.sign(kService, 'aws4_request')
       return kSigning

   def create_string_to_sign_v4(self, method='', canonical_uri='', canonical_query_string='', body='', epoch_t='',
                                algorithm='', host='', service='', region=''):
       """Generates string_to_sign for authorization key generation."""

       canonical_request = self.create_canonical_request(method, canonical_uri,canonical_query_string,
                                                    body, epoch_t, host)
       credential_scope = self.get_date(epoch_t) + '/' + \
           region + '/' + service + '/' + 'aws4_request'

       string_to_sign = algorithm + '\n' + self.get_amz_timestamp(epoch_t) + '\n' + credential_scope \
           + '\n' + hashlib.sha256(canonical_request.encode('utf-8')).hexdigest()
       return string_to_sign

   def sign_request_v4(self, method=None, canonical_uri='/', canonical_query_string='', body='', epoch_t='',
                       host='', service='', region=''):
       """Generate authorization request header."""
       if method is None:
           print("method can not be null")
           return None
       credential_scope = self.get_date(epoch_t) + '/' + region + \
           '/' + service + '/' + 'aws4_request'

       headers = self.get_headers(host, epoch_t, body)
       sorted_headers = sorted([k for k in headers])
       signed_headers = "{}".format(";".join(sorted_headers))

       algorithm = 'AWS4-HMAC-SHA256'

       if self._config.get_s3recovery_flag():
           access_key = self._config.get_s3_recovery_access_key()
           secret_key = self._config.get_s3_recovery_secret_key()
       else:
           access_key = self._config.get_cortx_s3_access_key()
           secret_key = self._config.get_cortx_s3_secret_key()

       string_to_sign = self.create_string_to_sign_v4(method, canonical_uri, canonical_query_string, body, epoch_t,
                                                 algorithm, host, service, region)

       signing_key = self.getV4SignatureKey(
           secret_key, self.get_date(epoch_t), region, service)

       signature = hmac.new(
           signing_key,
           (string_to_sign).encode('utf-8'),
           hashlib.sha256).hexdigest()

       authorization_header = algorithm + ' ' + 'Credential=' + access_key + '/' + \
           credential_scope + ', ' + 'SignedHeaders=' + signed_headers + \
           ', ' + 'Signature=' + signature
       return authorization_header

   def get_date(self, epoch_t):
       """Return date in Ymd format."""
       return epoch_t.strftime('%Y%m%d')

   def get_amz_timestamp(self, epoch_t):
       """Return timestamp in YMDTHMSZ format."""
       return epoch_t.strftime('%Y%m%dT%H%M%SZ')

   def prepare_signed_header(self, http_request, request_uri, query_params, body):
       """Generate headers used for authorization requests."""
       url_parse_result  = urllib.parse.urlparse(self._config.get_cortx_s3_endpoint())
       epoch_t = datetime.datetime.utcnow()
       headers = {'content-type': 'application/x-www-form-urlencoded',
               'Accept': 'text/plain'}
       headers['Authorization'] = self.sign_request_v4(http_request, request_uri ,query_params, body, epoch_t, url_parse_result.netloc,
           self._config.get_cortx_s3_service(), self._config.get_cortx_s3_region())
       headers['x-amz-date'] = self.get_amz_timestamp(epoch_t)
       headers['x-amz-content-sha256'] = self.body_hash_hex
       return headers
