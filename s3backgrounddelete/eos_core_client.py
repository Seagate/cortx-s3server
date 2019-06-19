import socket
import json
import xmltodict
import ssl
import http.client
import pprint
import sys
import os
import urllib

from config import Config

class EOSCoreClient:

    def __init__(self):
        pass

    def _get_connection(self):

        endpoint_url = urllib.parse.urlparse(Config.endpoint).netloc
        return http.client.HTTPConnection(endpoint_url)

    def put(self, request_uri, body=None, headers=None):

        conn = self._get_connection()
        if headers == None:
            headers = {"Content-type": "application/x-www-form-urlencoded", "Accept": "text/plain"}

        conn.request('PUT', request_uri, body, headers)
        response = conn.getresponse()
        result = {'status': response.status, 'headers': response.getheaders(),
                  'body': response.read(), 'reason': response.reason}
        conn.close()
        return result

    def get(self, request_uri, body=None, headers=None):

        conn = self._get_connection()
        if headers == None:
            headers = {"Content-type": "application/x-www-form-urlencoded", "Accept": "text/plain"}

        conn.request('GET', request_uri, body, headers)
        response = conn.getresponse()
        result = {'status': response.status, 'headers': response.getheaders(),
                  'body': response.read(), 'reason': response.reason}
        conn.close()
        return result

    def delete(self, request_uri, body=None, headers=None):

        conn = self._get_connection()
        if headers == None:
            headers = {"Content-type": "application/x-www-form-urlencoded", "Accept": "text/plain"}

        conn.request('DELETE', request_uri, body, headers)
        response = conn.getresponse()
        result = {'status': response.status, 'headers': response.getheaders(),
                  'body': response.read(), 'reason': response.reason}
        conn.close()
        return result
