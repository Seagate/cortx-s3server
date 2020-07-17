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

"""This class provides Index  REST API i.e. List, PUT."""

import logging
import urllib

from s3backgrounddelete.cortx_list_index_response import CORTXS3ListIndexResponse
from s3backgrounddelete.cortx_s3_client import CORTXS3Client
from s3backgrounddelete.cortx_s3_error_respose import CORTXS3ErrorResponse
from s3backgrounddelete.cortx_s3_success_response import CORTXS3SuccessResponse
from s3backgrounddelete.cortx_s3_util import CORTXS3Util
from s3backgrounddelete.IEMutil import IEMutil


# CORTXS3IndexApi supports index REST-API's List & Put

class CORTXS3IndexApi(CORTXS3Client):
    """CORTXS3IndexApi provides index REST-API's List and Put."""
    _logger = None

    def __init__(self, config, logger=None, connection=None):
        """Initialise logger and config."""
        if (logger is None):
            self._logger = logging.getLogger("CORTXS3IndexApi")
        else:
            self._logger = logger
        self.config = config
        self.s3_util = CORTXS3Util(self.config)

        #for testing scenarios, pass the mocked http connection object in init method..
        if (connection is None):
            super(CORTXS3IndexApi, self).__init__(self.config, logger = self._logger)
        else:
            super(CORTXS3IndexApi, self).__init__(self.config, logger = self._logger, connection=connection)


    def list(self, index_id, max_keys=1000, next_marker=None, additional_Query_params=None):
        """Perform LIST request and generate response."""
        if index_id is None:
            self._logger.error("Index Id is required.")
            return False, None

        self._logger.info("Processing request in IndexAPI")

        # The URL quoting functions focus on taking program data and making it safe for use as URL components by quoting special characters and appropriately encoding non-ASCII text.
        # https://docs.python.org/3/library/urllib.parse.html
        # For example if index_id is 'AAAAAAAAAHg=-AwAQAAAAAAA=' urllib.parse.quote(index_id, safe='') yields 'AAAAAAAAAHg%3D-AwAQAAAAAAA%3D'
        # And request_uri is '/indexes/AAAAAAAAAHg%3D-AwAQAAAAAAA%3D'

        request_uri = '/indexes/' + urllib.parse.quote(index_id, safe='')
        query_params = ""
        inputQueryParams = {}
        inputQueryParams["keys"] = max_keys
        if (next_marker is not None):
            inputQueryParams["marker"] = next_marker

        if (additional_Query_params is not None and isinstance(additional_Query_params, dict)):
            # Add addtional query params
            inputQueryParams.update(additional_Query_params)

        #Generate sorted urlencoded query params into query_params
        for key in sorted(inputQueryParams.keys()):
            d = {}
            d[key] = inputQueryParams[key]
            if (query_params == ""):
                query_params += urllib.parse.urlencode(d)
            else:
                query_params += "&" + urllib.parse.urlencode(d)

        absolute_request_uri = request_uri + '?' + query_params

        body = ""
        headers = self.s3_util.prepare_signed_header('GET', request_uri, query_params, body)

        if(headers['Authorization'] is None):
            self._logger.error("Failed to generate v4 signature")
            return False, None
        try:
            response = super(
                CORTXS3IndexApi,
                self).get(
                absolute_request_uri,
                headers=headers)
        except ConnectionRefusedError as ex:
            IEMutil("ERROR", IEMutil.S3_CONN_FAILURE, IEMutil.S3_CONN_FAILURE_STR)
            self._logger.error(repr(ex))
            return False, CORTXS3ErrorResponse(502,"","ConnectionRefused")
        except Exception as ex:
            self._logger.error(repr(ex))
            return False, CORTXS3ErrorResponse(500,"","InternalServerError")

        if response['status'] == 200:
            self._logger.info('Successfully listed Index details.')
            return True, CORTXS3ListIndexResponse(response['body'])
        else:
            self._logger.info('Failed to list Index details.')
            return False, CORTXS3ErrorResponse(
                response['status'], response['reason'], response['body'])

    def put(self, index_id):
        """Perform PUT request and generate response."""
        if index_id is None:
            self._logger.info("Index Id is required.")
            return False, None

        # The URL quoting functions focus on taking program data and making it safe for use as URL components by quoting special characters and appropriately encoding non-ASCII text.
        # https://docs.python.org/3/library/urllib.parse.html
        # For example if index_id is 'AAAAAAAAAHg=-AwAQAAAAAAA=' urllib.parse.quote(index_id, safe='') yields 'AAAAAAAAAHg%3D-AwAQAAAAAAA%3D'
        # And request_uri is '/indexes/AAAAAAAAAHg%3D-AwAQAAAAAAA%3D'

        request_uri = '/indexes/' + urllib.parse.quote(index_id, safe='')

        query_params = ""
        body = ""
        headers = self.s3_util.prepare_signed_header('PUT', request_uri, query_params, body)

        if(headers['Authorization'] is None):
            self._logger.error("Failed to generate v4 signature")
            return False, None

        try:
            response = super(
                CORTXS3IndexApi,
                self).put(
                request_uri,
                headers=headers)
        except ConnectionRefusedError as ex:
            IEMutil("ERROR", IEMutil.S3_CONN_FAILURE, IEMutil.S3_CONN_FAILURE_STR)
            self._logger.error(repr(ex))
            return False, CORTXS3ErrorResponse(502,"","ConnectionRefused")
        except Exception as ex:
            self._logger.error(repr(ex))
            return False, CORTXS3ErrorResponse(500,"","InternalServerError")

        if response['status'] == 201:
            self._logger.info('Successfully added Index.')
            return True, CORTXS3SuccessResponse(response['body'])
        else:
            self._logger.info('Failed to add Index.')
            return False, CORTXS3ErrorResponse(
                response['status'], response['reason'], response['body'])

    def delete(self, index_id):
        """Perform DELETE request and generate response."""
        if index_id is None:
            self._logger.info("Index Id is required.")
            return False, None

        # The URL quoting functions focus on taking program data and making it safe for use as URL components by quoting special characters and appropriately encoding non-ASCII text.
        # https://docs.python.org/3/library/urllib.parse.html
        # For example if index_id is 'AAAAAAAAAHg=-AwAQAAAAAAA=' urllib.parse.quote(index_id, safe='') yields 'AAAAAAAAAHg%3D-AwAQAAAAAAA%3D'
        # And request_uri is '/indexes/AAAAAAAAAHg%3D-AwAQAAAAAAA%3D'

        request_uri = '/indexes/' + urllib.parse.quote(index_id, safe='')

        query_params = ""
        body = ""
        headers = self.s3_util.prepare_signed_header('DELETE', request_uri, query_params, body)

        if(headers['Authorization'] is None):
            self._logger.error("Failed to generate v4 signature")
            return False, None

        try:
            response = super(
                CORTXS3IndexApi,
                self).delete(
                request_uri,
                headers=headers)
        except ConnectionRefusedError as ex:
            IEMutil("ERROR", IEMutil.S3_CONN_FAILURE, IEMutil.S3_CONN_FAILURE_STR)
            self._logger.error(repr(ex))
            return False, CORTXS3ErrorResponse(502,"","ConnectionRefused")
        except Exception as ex:
            self._logger.error(repr(ex))
            return False, CORTXS3ErrorResponse(500,"","InternalServerError")

        if response['status'] == 204:
            self._logger.info('Successfully deleted Index.')
            return True, CORTXS3SuccessResponse(response['body'])
        else:
            self._logger.info('Failed to delete Index.')
            return False, CORTXS3ErrorResponse(
                response['status'], response['reason'], response['body'])


    def head(self, index_id):
            """Perform HEAD request and generate response."""
            if index_id is None:
                self._logger.error("Index id is required.")
                return False, None

            # The URL quoting functions focus on taking program data and making it safe for use as URL components by quoting special characters and appropriately encoding non-ASCII text.
            # urllib.parse.urlencode converts a mapping object or a sequence of two-element tuples, which may contain str or bytes objects, to a percent-encoded ASCII text string.
            # https://docs.python.org/3/library/urllib.parse.html
            # For example if oid is 'JwZSAwAAAAA=-AgAAAAAA4Ag=' urllib.parse.quote(oid, safe='') yields 'JwZSAwAAAAA%3D-AgAAAAAA4Ag%3D'
            # And request_uri is '/indexes/JwZSAwAAAAA%3D-AgAAAAAA4Ag%3D'

            request_uri = '/indexes/' + urllib.parse.quote(index_id, safe='')

            query_params = ""
            body = ""
            headers = self.s3_util.prepare_signed_header('HEAD', request_uri, query_params, body)

            if headers['Authorization'] is None:
                self._logger.error("Failed to generate v4 signature")
                return False, None

            try:
                response = super(
                    CORTXS3IndexApi,
                    self).head(
                    request_uri,
                    body,
                    headers=headers)
            except ConnectionRefusedError as ex:
                IEMutil("ERROR", IEMutil.S3_CONN_FAILURE, IEMutil.S3_CONN_FAILURE_STR)
                self._logger.error(repr(ex))
                return False, CORTXS3ErrorResponse(502,"","ConnectionRefused")
            except Exception as ex:
                self._logger.error(repr(ex))
                return False, CORTXS3ErrorResponse(500,"","InternalServerError")

            if response['status'] == 200:
                self._logger.info("HEAD Index called successfully with status code: "\
                    + str(response['status']) + " response body: " + str(response['body']))
                return True, CORTXS3SuccessResponse(response['body'])
            else:
                self._logger.info("Failed to do HEAD Index with status code: "\
                    + str(response['status']) + " response body: " + str(response['body']))
                return False, CORTXS3ErrorResponse(
                    response['status'], response['reason'], response['body'])
