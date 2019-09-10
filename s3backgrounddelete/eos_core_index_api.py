"""This class provides Index  REST API i.e. List, PUT."""

import logging
import urllib

from eos_list_index_response import EOSCoreListIndexResponse
from eos_core_client import EOSCoreClient
from eos_core_error_respose import EOSCoreErrorResponse
from eos_core_success_response import EOSCoreSuccessResponse
from eos_core_util import EOSCoreUtil


# EOSCoreIndexApi supports index REST-API's List & Put

class EOSCoreIndexApi(EOSCoreClient):
    """EOSCoreIndexApi provides index REST-API's List and Put."""
    _logger = None

    def __init__(self, config, logger=None, connection=None):
        """Initialise logger and config."""
        if (logger is None):
            self._logger = logging.getLogger("EOSCoreIndexApi")
        else:
            self._logger = logger
        self.config = config
        #for testing scenarios, pass the mocked http connection object in init method..
        if (connection is None):
            super(EOSCoreIndexApi, self).__init__(self.config, logger = self._logger)
        else:
            super(EOSCoreIndexApi, self).__init__(self.config, logger = self._logger, connection=connection)


    def list(self, index_id, max_keys=1000, next_marker=None):
        """Perform LIST request and generate response."""
        if index_id is None:
            self._logger.error("Index Id is required.")
            return None

        self._logger.info("Processing request in IndexAPI")

        # The URL quoting functions focus on taking program data and making it safe for use as URL components by quoting special characters and appropriately encoding non-ASCII text.
        # https://docs.python.org/3/library/urllib.parse.html
        # For example if index_id is 'AAAAAAAAAHg=-AwAQAAAAAAA=' urllib.parse.quote(index_id) yields 'AAAAAAAAAHg%3D-AwAQAAAAAAA%3D'
        # And request_uri is '/indexes/AAAAAAAAAHg%3D-AwAQAAAAAAA%3D'

        request_uri = '/indexes/' + urllib.parse.quote(index_id)
        query_params = ""

        if (next_marker is not None):
            query_params = urllib.parse.urlencode({'max-keys': max_keys,'marker': next_marker})
            absolute_request_uri = request_uri + '?' + query_params
        else:
            query_params = urllib.parse.urlencode({'max-keys': max_keys})
            absolute_request_uri = request_uri + '?' + query_params

        body = ""
        headers = EOSCoreUtil.prepare_signed_header('GET', request_uri, query_params, body)

        if(headers['Authorization'] is None):
            self._logger.error("Failed to generate v4 signature")
            return None
        try:
            response = super(
                EOSCoreIndexApi,
                self).get(
                absolute_request_uri,
                headers=headers)
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

    def put(self, index_id):
        """Perform PUT request and generate response."""
        if index_id is None:
            self._logger.info("Index Id is required.")
            return None

        # The URL quoting functions focus on taking program data and making it safe for use as URL components by quoting special characters and appropriately encoding non-ASCII text.
        # https://docs.python.org/3/library/urllib.parse.html
        # For example if index_id is 'AAAAAAAAAHg=-AwAQAAAAAAA=' urllib.parse.quote(index_id) yields 'AAAAAAAAAHg%3D-AwAQAAAAAAA%3D'
        # And request_uri is '/indexes/AAAAAAAAAHg%3D-AwAQAAAAAAA%3D'

        request_uri = '/indexes/' + urllib.parse.quote(index_id)

        query_params = ""
        body = ""
        headers = EOSCoreUtil.prepare_signed_header('PUT', request_uri, query_params, body)

        if(headers['Authorization'] is None):
            self._logger.error("Failed to generate v4 signature")
            return None

        try:
            response = super(
                EOSCoreIndexApi,
                self).put(
                request_uri,
                headers=headers)
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
