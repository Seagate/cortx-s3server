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

"""This class provides Object  REST API i.e. GET,PUT,DELETE and HEAD."""
import logging
import urllib

from s3backgrounddelete.cortx_s3_error_respose import CORTXS3ErrorResponse
from s3backgrounddelete.cortx_s3_success_response import CORTXS3SuccessResponse
from s3backgrounddelete.cortx_s3_client import CORTXS3Client
from s3backgrounddelete.cortx_s3_util import CORTXS3Util
from s3backgrounddelete.IEMutil import IEMutil

# CORTXS3ObjectApi supports object REST-API's Put, Get & Delete
class CORTXS3ObjectApi(CORTXS3Client):
    """CORTXS3ObjectApi provides object REST-API's Get, Put and Delete."""
    _logger = None

    def __init__(self, config, logger=None, connection=None):
        """Initialise logger and config."""
        if (logger is None):
            self._logger = logging.getLogger("CORTXS3ObjectApi")
        else:
            self._logger = logger
        self.config = config
        self.s3_util = CORTXS3Util(self.config)

        if (connection is None):
            super(CORTXS3ObjectApi, self).__init__(self.config, logger = self._logger)
        else:
            super(CORTXS3ObjectApi, self).__init__(self.config, logger=self._logger, connection=connection)


    def put(self, oid, value):
        """Perform PUT request and generate response."""
        if oid is None:
            self._logger.error("Object Id is required.")
            return False, None

        query_params = ""
        request_body = value

        # The URL quoting functions focus on taking program data and making it safe for use as URL components by quoting special characters and appropriately encoding non-ASCII text.
        # https://docs.python.org/3/library/urllib.parse.html
        # For example if oid is 'JwZSAwAAAAA=-AgAAAAAA4Ag=' urllib.parse.quote(oid, safe='') yields 'JwZSAwAAAAA%3D-AgAAAAAA4Ag%3D'
        # And request_uri is '/objects/JwZSAwAAAAA%3D-AgAAAAAA4Ag%3D'

        request_uri = '/objects/' + urllib.parse.quote(oid, safe='')

        headers = self.s3_util.prepare_signed_header('PUT', request_uri, query_params, request_body)

        if headers['Authorization'] is None:
            self._logger.error("Failed to generate v4 signature")
            return False, None

        try:
            response = super(
                CORTXS3ObjectApi,
                self).put(
                request_uri,
                request_body,
                headers=headers)
        except ConnectionRefusedError as ex:
            IEMutil("ERROR", IEMutil.S3_CONN_FAILURE, IEMutil.S3_CONN_FAILURE_STR)
            self._logger.error(repr(ex))
            return False, CORTXS3ErrorResponse(502,"","ConnectionRefused")
        except Exception as ex:
            self._logger.error(repr(ex))
            return False, CORTXS3ErrorResponse(500,"","InternalServerError")

        if response['status'] == 201:
            self._logger.info("Object added successfully.")
            return True, CORTXS3SuccessResponse(response['body'])
        else:
            self._logger.info('Failed to add Object.')
            return False, CORTXS3ErrorResponse(
                response['status'], response['reason'], response['body'])

    def get(self, oid):
        """Perform GET request and generate response."""
        if oid is None:
            self._logger.error("Object Id is required.")
            return False, None
        request_uri = '/objects/' + urllib.parse.quote(oid, safe='')

        query_params = ""
        body = ""

        # The URL quoting functions focus on taking program data and making it safe for use as URL components by quoting special characters and appropriately encoding non-ASCII text.
        # https://docs.python.org/3/library/urllib.parse.html
        # For example if oid is 'JwZSAwAAAAA=-AgAAAAAA4Ag=' urllib.parse.quote(oid, safe='') yields 'JwZSAwAAAAA%3D-AgAAAAAA4Ag%3D'
        # And request_uri is '/objects/JwZSAwAAAAA%3D-AgAAAAAA4Ag%3D'

        headers = self.s3_util.prepare_signed_header('GET', request_uri, query_params, body)

        if headers['Authorization'] is None:
            self._logger.error("Failed to generate v4 signature")
            return False, None

        try:
            response = super(
                CORTXS3ObjectApi,
                self).get(
                request_uri,
                headers=headers)
        except ConnectionRefusedError as ex:
            IEMutil("ERROR", IEMutil.S3_CONN_FAILURE, IEMutil.S3_CONN_FAILURE_STR)
            self._logger.error(repr(ex))
            return False, CORTXS3ErrorResponse(502,"","ConnectionRefused")
        except Exception as ex:
            self._logger.error(repr(ex))
            return False, CORTXS3ErrorResponse(500,"","InternalServerError")

        if response['status'] == 200:
            self._logger.info('Successfully fetched object details.')
            return True, CORTXS3SuccessResponse(response['body'])
        else:
            self._logger.info('Failed to fetch object details.')
            return False, CORTXS3ErrorResponse(
                response['status'], response['reason'], response['body'])

    def delete(self, oid, layout_id):
        """Perform DELETE request and generate response."""
        if oid is None:
            self._logger.error("Object Id is required.")
            return False, None
        if layout_id is None:
            self._logger.error("Layout Id is required.")
            return False, None

        # The URL quoting functions focus on taking program data and making it safe for use as URL components by quoting special characters and appropriately encoding non-ASCII text.
        # urllib.parse.urlencode converts a mapping object or a sequence of two-element tuples, which may contain str or bytes objects, to a percent-encoded ASCII text string.
        # https://docs.python.org/3/library/urllib.parse.html
        # For example if oid is 'JwZSAwAAAAA=-AgAAAAAA4Ag=' urllib.parse.quote(oid, safe='') yields 'JwZSAwAAAAA%3D-AgAAAAAA4Ag%3D' and layout_id is 1
        # urllib.parse.urlencode({'layout-id': layout_id}) yields layout-id=1
        # And request_uri is '/objects/JwZSAwAAAAA%3D-AgAAAAAA4Ag%3D' ,
        # absolute_request_uri is layout-id is '/objects/JwZSAwAAAAA%3D-AgAAAAAA4Ag%3D?layout-id=1'

        query_params = urllib.parse.urlencode({'layout-id': layout_id})
        request_uri = '/objects/' + urllib.parse.quote(oid, safe='')
        absolute_request_uri = request_uri + '?' + query_params

        body = ''
        headers = self.s3_util.prepare_signed_header('DELETE', request_uri, query_params, body)

        if headers['Authorization'] is None:
            self._logger.error("Failed to generate v4 signature")
            return False, None

        try:
            response = super(
                CORTXS3ObjectApi,
                self).delete(
                absolute_request_uri,
                body,
                headers=headers)
        except ConnectionRefusedError as ex:
            IEMutil("ERROR", IEMutil.S3_CONN_FAILURE, IEMutil.S3_CONN_FAILURE_STR)
            self._logger.error(repr(ex))
            return False, CORTXS3ErrorResponse(502,"","ConnectionRefused")
        except Exception as ex:
            self._logger.error(repr(ex))
            return False, CORTXS3ErrorResponse(500,"","InternalServerError")

        if response['status'] == 204:
            self._logger.info('Object deleted successfully.')
            return True, CORTXS3SuccessResponse(response['body'])
        else:
            self._logger.info('Failed to delete Object.')
            return False, CORTXS3ErrorResponse(
                response['status'], response['reason'], response['body'])


    def head(self, oid, layout_id):
        """Perform HEAD request and generate response."""
        if oid is None:
            self._logger.error("Object Id is required.")
            return False, None
        if layout_id is None:
            self._logger.error("Layout Id is required.")
            return False, None

        # The URL quoting functions focus on taking program data and making it safe for use as URL components by quoting special characters and appropriately encoding non-ASCII text.
        # urllib.parse.urlencode converts a mapping object or a sequence of two-element tuples, which may contain str or bytes objects, to a percent-encoded ASCII text string.
        # https://docs.python.org/3/library/urllib.parse.html
        # For example if oid is 'JwZSAwAAAAA=-AgAAAAAA4Ag=' urllib.parse.quote(oid, safe='') yields 'JwZSAwAAAAA%3D-AgAAAAAA4Ag%3D' and layout_id is 1
        # urllib.parse.urlencode({'layout-id': layout_id}) yields layout-id=1
        # And request_uri is '/objects/JwZSAwAAAAA%3D-AgAAAAAA4Ag%3D' ,
        # absolute_request_uri is layout-id is '/objects/JwZSAwAAAAA%3D-AgAAAAAA4Ag%3D?layout-id=1'

        query_params = urllib.parse.urlencode({'layout-id': layout_id})
        request_uri = '/objects/' + urllib.parse.quote(oid, safe='')
        absolute_request_uri = request_uri + '?' + query_params

        body = ''
        headers = self.s3_util.prepare_signed_header('HEAD', request_uri, query_params, body)

        if headers['Authorization'] is None:
            self._logger.error("Failed to generate v4 signature")
            return False, None

        try:
            response = super(
                CORTXS3ObjectApi,
                self).head(
                absolute_request_uri,
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
            self._logger.info("HEAD Object called successfully with status code: "\
                 + str(response['status']) + " response body: " + str(response['body']))
            return True, CORTXS3SuccessResponse(response['body'])
        else:
            self._logger.info("Failed to do HEAD Object with status code: "\
                 + str(response['status']) + " response body: " + str(response['body']))
            return False, CORTXS3ErrorResponse(
                response['status'], response['reason'], response['body'])

