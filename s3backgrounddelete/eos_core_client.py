import socket
import json
import xmltodict
import ssl
import http.client
import pprint
import sys
import os
import urllib
import logging

class EOSCoreClient:

    _config = None
    _logger = None

    def __init__(self, config, logger = None):
        if (logger is None):
            self._logger = logging.getLogger("EOSCoreClient")
        else:
            self._logger = logger
        self._config = config

    def _get_connection(self):
        try:
            endpoint_url = urllib.parse.urlparse( self._config.get_eos_core_endpoint() ).netloc
        except KeyError as ex:
            self._logger.error(str(ex))
            return None
        return http.client.HTTPConnection(endpoint_url)

    def put(self, request_uri, body=None, headers=None):

        conn = self._get_connection()
        if ( conn is None ):
            raise TypeError("Failed to create connection instance")
        if ( headers is None ):
            headers = {"Content-type": "application/x-www-form-urlencoded", "Accept": "text/plain"}

        conn.request('PUT', request_uri, body, headers)
        response = conn.getresponse()
        result = {'status': response.status, 'headers': response.getheaders(),
                  'body': response.read(), 'reason': response.reason}
        conn.close()
        return result

    def get(self, request_uri, body=None, headers=None):

        conn = self._get_connection()
        if ( conn is None ):
            raise TypeError("Failed to create connection instance")
        if ( headers is None ):
            headers = {"Content-type": "application/x-www-form-urlencoded", "Accept": "text/plain"}

        conn.request('GET', request_uri, body, headers)
        response = conn.getresponse()
        result = {'status': response.status, 'headers': response.getheaders(),
                  'body': response.read(), 'reason': response.reason}
        conn.close()
        return result

    def delete(self, request_uri, body=None, headers=None):

        conn = self._get_connection()
        if ( conn is None ):
            raise TypeError("Failed to create connection instance")
        if ( headers is None ):
            headers = {"Content-type": "application/x-www-form-urlencoded", "Accept": "text/plain"}

        conn.request('DELETE', request_uri, body, headers)
        response = conn.getresponse()
        result = {'status': response.status, 'headers': response.getheaders(),
                  'body': response.read(), 'reason': response.reason}
        conn.close()
        return result
