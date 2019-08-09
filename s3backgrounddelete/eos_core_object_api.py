"""This class provides Object  REST API i.e. GET,PUT and DELETE."""
import logging
import urllib

from eos_core_error_respose import EOSCoreErrorResponse
from eos_core_success_response import EOSCoreSuccessResponse
from eos_core_client import EOSCoreClient
from eos_core_util import prepare_signed_header

# EOSCoreObjectApi supports object REST-API's Put, Get & Delete


class EOSCoreObjectApi(EOSCoreClient):
    """EOSCoreObjectApi provides object REST-API's Get, Put and Delete."""
    _logger = None

    def __init__(self, config, logger=None):
        """Initialise logger and config."""
        if (logger is None):
            self._logger = logging.getLogger("EOSCoreObjectApi")
        else:
            self._logger = logger
        self.config = config
        super(EOSCoreObjectApi, self,).__init__(self.config, logger=self._logger)

    def put(self, oid, value):
        """Perform PUT request and generate response."""
        if oid is None:
            self._logger.error("Object Id is required.")
            return

        query_params = ""
        request_body = value
        request_uri = '/objects/' + oid

        headers = prepare_signed_header('PUT', request_uri, query_params, request_body)

        if(headers['Authorization'] is None):
            self._logger.error("Failed to generate v4 signature")
            return None

        try:
            response = super(EOSCoreObjectApi, self).put(request_uri, request_body, headers = headers)
        except Exception as ex:
            self._logger.error(str(ex))
            return None

        if response['status'] == 201:
            self._logger.info("Object added successfully.")
            return True, EOSCoreSuccessResponse(response['body'])
        else:
            self._logger.info('Failed to add Object.')
            return False, EOSCoreErrorResponse(
                response['status'], response['reason'], response['body'])

    def get(self, oid):
        """Perform GET request and generate response."""
        if oid is None:
            self._logger.error("Object Id is required.")
            return
        request_uri = '/objects/' + oid

        query_params = ""
        body = ""
        headers = prepare_signed_header('GET', request_uri, query_params, body)

        if(headers['Authorization'] is None):
            self._logger.error("Failed to generate v4 signature")
            return None

        try:
            response = super(EOSCoreObjectApi, self).get(request_uri, headers = headers)
        except Exception as ex:
            self._logger.error(str(ex))
            return None

        if response['status'] == 200:
            self._logger.info('Successfully fetched object details.')
            return True, EOSCoreSuccessResponse(response['body'])
        else:
            self._logger.info('Failed to fetch object details.')
            return False, EOSCoreErrorResponse(
                response['status'], response['reason'], response['body'])

    def delete(self, oid, layout_id):
        """Perform DELETE request and generate response."""
        if oid is None:
            self._logger.error("Object Id is required.")
            return
        if layout_id is None:
            self._logger.error("Layout Id is required.")
            return


        query_params = urllib.parse.urlencode({'layout-id': layout_id})
        request_uri = '/objects/' + oid
        absolute_request_uri = request_uri + '?' + query_params

        body = ''
        headers = prepare_signed_header('DELETE', request_uri, query_params, body)

        if(headers['Authorization'] is None):
            self._logger.error("Failed to generate v4 signature")
            return None

        try:
            response = super(EOSCoreObjectApi, self).delete(absolute_request_uri, body , headers = headers)
        except Exception as ex:
            self._logger.error(str(ex))
            return None

        if response['status'] == 204:
            self._logger.info('Object deleted successfully.')
            return True, EOSCoreSuccessResponse(response['body'])
        else:
            self._logger.info('Failed to delete Object.')
            return False, EOSCoreErrorResponse(
                response['status'], response['reason'], response['body'])
