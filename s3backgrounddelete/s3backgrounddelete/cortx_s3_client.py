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

"""This is an s3 client which will do GET,PUT,DELETE and HEAD requests."""

import sys
import logging
import http.client
import urllib


class CORTXS3Client(object):
    """creates s3 client."""
    _config = None
    _logger = None
    _conn = None

    def __init__(self, config, logger=None, connection=None):
        """Initialise s3 client using config, connection object and logger."""
        if (logger is None):
            self._logger = logging.getLogger("CORTXS3Client")
        else:
            self._logger = logger
        self._config = config
        if (connection is None):
            self._conn = self._get_connection()
        else:
            self._conn = connection

    def _get_connection(self):
        """Creates new connection."""
        try:
            endpoint_url = urllib.parse.urlparse(
                self._config.get_cortx_s3_endpoint()).netloc
        except KeyError as ex:
            self._logger.error(str(ex))
            return None
        return http.client.HTTPConnection(endpoint_url)

    def put(self, request_uri, body=None, headers=None):
        """Perform PUT request and generate response."""
        if (self._conn is None):
            raise TypeError("Failed to create connection instance")
        if (headers is None):
            headers = {
                "Content-type": "application/x-www-form-urlencoded",
                "Accept": "text/plain"}

        self._conn.request('PUT', request_uri, body, headers)
        response = self._conn.getresponse()
        result = {'status': response.status, 'headers': response.getheaders(),
                  'body': response.read(), 'reason': response.reason}
        self._conn.close()
        return result

    def get(self, request_uri, body=None, headers=None):
        """Perform GET request and generate response."""
        if (self._conn is None):
            raise TypeError("Failed to create connection instance")
        if (headers is None):
            headers = {
                "Content-type": "application/x-www-form-urlencoded",
                "Accept": "text/plain"}

        self._conn.request('GET', request_uri, body, headers)
        response = self._conn.getresponse()
        result = {'status': response.status, 'headers': response.getheaders(),
                  'body': response.read(), 'reason': response.reason}
        self._conn.close()
        return result

    def delete(self, request_uri, body=None, headers=None):
        """Perform DELETE request and generate response."""
        if (self._conn is None):
            raise TypeError("Failed to create connection instance")
        if (headers is None):
            headers = {
                "Content-type": "application/x-www-form-urlencoded",
                "Accept": "text/plain"}

        self._conn.request('DELETE', request_uri, body, headers)
        response = self._conn.getresponse()
        result = {'status': response.status, 'headers': response.getheaders(),
                  'body': response.read(), 'reason': response.reason}
        self._conn.close()
        return result

    def head(self, request_uri, body=None, headers=None):
        """Perform HEAD request and generate response."""
        if (self._conn is None):
            raise TypeError("Failed to create connection instance")
        if (headers is None):
            headers = {
                "Content-type": "application/x-www-form-urlencoded",
                "Accept": "text/plain"}

        self._conn.request('HEAD', request_uri, body, headers)
        response = self._conn.getresponse()
        result = {'status': response.status, 'headers': response.getheaders(),
                  'body': response.read(), 'reason': response.reason}
        self._conn.close()
        return result

