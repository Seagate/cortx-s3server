"""This class provides Index  REST API i.e. List, PUT."""

import logging

from eos_list_index_response import EOSCoreListIndexResponse
from eos_core_client import EOSCoreClient
from eos_core_error_respose import EOSCoreErrorResponse
from eos_core_success_response import EOSCoreSuccessResponse
from eos_core_util import prepare_signed_header


# EOSCoreIndexApi supports index REST-API's List & Put


class EOSCoreIndexApi(EOSCoreClient):
    """EOSCoreIndexApi provides index REST-API's List and Put."""
    _logger = None

    def __init__(self, config, logger=None):
        """Initialise logger and config."""
        if (logger is None):
            self._logger = logging.getLogger("EOSCoreIndexApi")
        else:
            self._logger = logger
        self.config = config
        super(EOSCoreIndexApi, self).__init__(self.config, logger=self._logger)

    def list(self, index):
        """Perform LIST request and generate response."""
        if index is None:
            self._logger.error("Index Id is required.")
            return None

        self._logger.info("Processing request in IndexAPI")
        request_uri = '/indexes/' + index

        query_params = ""
        body = ""
        headers = prepare_signed_header('GET', request_uri, query_params, body)

        if(headers['Authorization'] is None):
            self._logger.error("Failed to generate v4 signature")
            return None
        try:
            response = super(EOSCoreIndexApi, self).get(request_uri, headers = headers)
        except Exception as ex:
            self._logger.error(str(ex))
            return None

        if response['status'] == 200:
            self._logger.info('Successfully listed Index details.')
            return True, EOSCoreListIndexResponse(response['body'])
        else:
            self._logger.info('Failed to list Index details.')
            return False, EOSCoreErrorResponse(
                response['status'], response['reason'], response['body'])

    def put(self, index):
        """Perform PUT request and generate response."""
        if index is None:
            self._logger.info("Index Id is required.")
            return None

        request_uri = '/indexes/' + index

        query_params = ""
        body = ""
        headers = prepare_signed_header('PUT', request_uri, query_params, body)

        if(headers['Authorization'] is None):
            self._logger.error("Failed to generate v4 signature")
            return None

        try:
            response = super(EOSCoreIndexApi, self).put(request_uri, headers = headers)
        except Exception as ex:
            self._logger.error(str(ex))
            return None

        if response['status'] == 201:
            self._logger.info('Successfully added Index.')
            return True, EOSCoreSuccessResponse(response['body'])
        else:
            self._logger.info('Failed to add Index.')
            return False, EOSCoreErrorResponse(
                response['status'], response['reason'], response['body'])
