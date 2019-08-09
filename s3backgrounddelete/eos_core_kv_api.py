"""This class provides Key-value REST API i.e. GET,PUT and DELETE."""
import logging

from eos_core_client import EOSCoreClient
from eos_get_kv_response import EOSCoreGetKVResponse
from eos_core_error_respose import EOSCoreErrorResponse
from eos_core_success_response import EOSCoreSuccessResponse
from eos_core_util import prepare_signed_header

# EOSCoreKVApi supports key-value REST-API's Put, Get & Delete


class EOSCoreKVApi(EOSCoreClient):
    """EOSCoreKVApi provides key-value REST-API's Put, Get & Delete."""
    _logger = None

    def __init__(self, config, logger=None):
        """Initialise logger and config."""
        if (logger is None):
            self._logger = logging.getLogger("EOSCoreKVApi")
        else:
            self._logger = logger
        self._logger = logging.getLogger()
        self.config = config
        super(EOSCoreKVApi, self).__init__(self.config, logger=self._logger)

    def put(self, index=None, key=None, value=None):
        """Perform PUT request and generate response."""
        if index is None:
            self._logger.error("Index Id is required.")
            return None
        if key is None:
            self._logger.error("Key is required")
            return None

        query_params = ""
        request_body = value
        request_uri = '/indexes/' + index + '/' + key
        headers = prepare_signed_header('PUT', request_uri, query_params, request_body)

        if(headers['Authorization'] is None):
            self._logger.error("Failed to generate v4 signature")
            return None

        try:
            response = super(EOSCoreKVApi, self).put(request_uri, request_body, headers = headers)
        except Exception as ex:
            self._logger.error(str(ex))
            return None

        if response['status'] == 201:
            self._logger.info("Key value details added successfully.")
            return True, EOSCoreSuccessResponse(response['body'])
        else:
            self._logger.info('Failed to add key value details.')
            return False, EOSCoreErrorResponse(
                response['status'], response['reason'], response['body'])

    def get(self, index=None, key=None):
        """Perform GET request and generate response."""
        if index is None:
            self._logger.error("Index Id is required.")
            return None
        if key is None:
            self._logger.error("Key is required")
            return None

        request_uri = '/indexes/' + index + '/' + key

        query_params = ""
        body = ""
        headers = prepare_signed_header('GET', request_uri, query_params, body)

        if(headers['Authorization'] is None):
            self._logger.error("Failed to generate v4 signature")
            return None

        try:
            response = super(EOSCoreKVApi, self).get(request_uri, headers = headers)
        except Exception as ex:
            self._logger.error(str(ex))
            return None

        if response['status'] == 200:
            self._logger.info("Get kv operation successfully.")
            return True, EOSCoreGetKVResponse(key, response['body'])
        else:
            self._logger.info('Failed to get kv details.')
            return False, EOSCoreErrorResponse(
                response['status'], response['reason'], response['body'])

    def delete(self, index=None, key=None):
        """Perform DELETE request and generate response."""
        if index is None:
            self._logger.error("Index Id is required.")
            return None
        if key is None:
            self._logger.error("Key is required")
            return None

        request_uri = '/indexes/' + index + '/' + key

        body = ""
        query_params = ""
        headers = prepare_signed_header('DELETE', request_uri, query_params, body)

        if(headers['Authorization'] is None):
            self._logger.error("Failed to generate v4 signature")
            return None

        try:
            response = super(EOSCoreKVApi, self).delete(request_uri, headers = headers)
        except Exception as ex:
            self._logger.error(str(ex))
            return None

        if response['status'] == 204:
            self._logger.info('Key value deleted.')
            return True, EOSCoreSuccessResponse(response['body'])
        else:
            self._logger.info('Failed to delete key value.')
            return False, EOSCoreErrorResponse(
                response['status'], response['reason'], response['body'])
