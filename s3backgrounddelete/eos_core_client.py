"""This is an core client which will do GET,PUT,DELETE requests."""

import sys
import logging
import http.client
import urllib


class EOSCoreClient(object):
    """creates core client."""
    _config = None
    _logger = None

    def __init__(self, config, logger=None):
        """Initialise new logger or use existing log configs."""
        if (logger is None):
            self._logger = logging.getLogger("EOSCoreClient")
        else:
            self._logger = logger
        self._config = config

    def _get_connection(self):
        """Creates new connection."""
        try:
            endpoint_url = urllib.parse.urlparse(
                self._config.get_eos_core_endpoint()).netloc
        except KeyError as ex:
            self._logger.error(str(ex))
            return None
        return http.client.HTTPConnection(endpoint_url)

    def put(self, request_uri, body=None, headers=None):
        """Perform PUT request and generate response."""

        conn = self._get_connection()
        if (conn is None):
            raise TypeError("Failed to create connection instance")
        if (headers is None):
            headers = {
                "Content-type": "application/x-www-form-urlencoded",
                "Accept": "text/plain"}

        conn.request('PUT', request_uri, body, headers)
        response = conn.getresponse()
        result = {'status': response.status, 'headers': response.getheaders(),
                  'body': response.read(), 'reason': response.reason}
        conn.close()
        return result

    def get(self, request_uri, body=None, headers=None):
        """Perform GET request and generate response."""
        conn = self._get_connection()
        if (conn is None):
            raise TypeError("Failed to create connection instance")
        if (headers is None):
            headers = {
                "Content-type": "application/x-www-form-urlencoded",
                "Accept": "text/plain"}

        conn.request('GET', request_uri, body, headers)
        response = conn.getresponse()
        result = {'status': response.status, 'headers': response.getheaders(),
                  'body': response.read(), 'reason': response.reason}
        conn.close()
        return result

    def delete(self, request_uri, body=None, headers=None):
        """Perform DELETE request and generate response."""
        conn = self._get_connection()
        if (conn is None):
            raise TypeError("Failed to create connection instance")
        if (headers is None):
            headers = {
                "Content-type": "application/x-www-form-urlencoded",
                "Accept": "text/plain"}

        conn.request('DELETE', request_uri, body, headers)
        response = conn.getresponse()
        result = {'status': response.status, 'headers': response.getheaders(),
                  'body': response.read(), 'reason': response.reason}
        conn.close()
        return result
