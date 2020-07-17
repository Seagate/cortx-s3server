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

"""This class provides Key-value REST API i.e. GET,PUT and DELETE."""
import logging
import urllib

from s3backgrounddelete.cortx_s3_client import CORTXS3Client
from s3backgrounddelete.cortx_get_kv_response import CORTXS3GetKVResponse
from s3backgrounddelete.cortx_s3_error_respose import CORTXS3ErrorResponse
from s3backgrounddelete.cortx_s3_success_response import CORTXS3SuccessResponse
from s3backgrounddelete.cortx_s3_util import CORTXS3Util
from s3backgrounddelete.IEMutil import IEMutil

# CORTXS3KVApi supports key-value REST-API's Put, Get & Delete
class CORTXS3KVApi(CORTXS3Client):
    """CORTXS3KVApi provides key-value REST-API's Put, Get & Delete."""
    _logger = None

    def __init__(self, config, logger=None, connection=None):
        """Initialise logger and config."""
        if (logger is None):
            self._logger = logging.getLogger("CORTXS3KVApi")
        else:
            self._logger = logger
        self._logger = logging.getLogger()
        self.config = config
        self.s3_util = CORTXS3Util(self.config)

        if (connection is None):
            super(CORTXS3KVApi, self).__init__(self.config, logger=self._logger)
        else:
            super(CORTXS3KVApi, self).__init__(self.config, logger=self._logger, connection=connection)


    def put(self, index_id=None, object_key_name=None, value=""):
        """Perform PUT request and generate response."""
        if index_id is None:
            self._logger.error("Index Id is required.")
            return False, None
        if object_key_name is None:
            self._logger.error("Key is required")
            return False, None

        query_params = ""
        request_body = value

        # The URL quoting functions focus on taking program data and making it safe for use as URL components by quoting special characters and appropriately encoding non-ASCII text.
        # https://docs.python.org/3/library/urllib.parse.html
        # For example if index_id is 'AAAAAAAAAHg=-AwAQAAAAAAA=' and object_key_name is "testobject+"
        # urllib.parse.quote(index_id, safe='') and urllib.parse.quote(object_key_name) yields 'testobject%2B' respectively
        # And request_uri is
        # '/indexes/AAAAAAAAAHg%3D-AwAQAAAAAAA%3D/testobject%2B'

        request_uri = '/indexes/' + \
            urllib.parse.quote(index_id, safe='') + '/' + \
            urllib.parse.quote(object_key_name)
        headers = self.s3_util.prepare_signed_header('PUT', request_uri, query_params, request_body)

        if(headers['Authorization'] is None):
            self._logger.error("Failed to generate v4 signature")
            return False, None

        try:
            response = super(
                CORTXS3KVApi,
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



        if response['status'] == 200:
            self._logger.info("Key value details added successfully.")
            return True, CORTXS3SuccessResponse(response['body'])
        else:
            self._logger.info('Failed to add key value details.')
            return False, CORTXS3ErrorResponse(
                response['status'], response['reason'], response['body'])

    def get(self, index_id=None, object_key_name=None):
        """Perform GET request and generate response."""
        if index_id is None:
            self._logger.error("Index Id is required.")
            return False, None
        if object_key_name is None:
            self._logger.error("Key is required")
            return False, None

        # The URL quoting functions focus on taking program data and making it safe for use as URL components by quoting special characters and appropriately encoding non-ASCII text.
        # https://docs.python.org/3/library/urllib.parse.html
        # For example if index_id is 'AAAAAAAAAHg=-AwAQAAAAAAA=' and object_key_name is "testobject+"
        # urllib.parse.quote(index_id, safe='') and urllib.parse.quote(object_key_name) yields 'testobject%2B' respectively
        # And request_uri is
        # '/indexes/AAAAAAAAAHg%3D-AwAQAAAAAAA%3D/testobject%2B'

        request_uri = '/indexes/' + \
            urllib.parse.quote(index_id, safe='') + '/' + \
            urllib.parse.quote(object_key_name)

        query_params = ""
        body = ""
        headers = self.s3_util.prepare_signed_header('GET', request_uri, query_params, body)

        if(headers['Authorization'] is None):
            self._logger.error("Failed to generate v4 signature")
            return False, None

        try:
            response = super(
                CORTXS3KVApi,
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
            self._logger.info("Get kv operation successfully.")
            return True, CORTXS3GetKVResponse(
                object_key_name, response['body'])
        else:
            self._logger.info('Failed to get kv details.')
            return False, CORTXS3ErrorResponse(
                response['status'], response['reason'], response['body'])

    def delete(self, index_id=None, object_key_name=None):
        """Perform DELETE request and generate response."""
        if index_id is None:
            self._logger.error("Index Id is required.")
            return False, None
        if object_key_name is None:
            self._logger.error("Key is required")
            return False, None

        # The URL quoting functions focus on taking program data and making it safe for use as URL components by quoting special characters and appropriately encoding non-ASCII text.
        # https://docs.python.org/3/library/urllib.parse.html
        # For example if index_id is 'AAAAAAAAAHg=-AwAQAAAAAAA=' and object_key_name is "testobject+"
        # urllib.parse.quote(index_id, safe='') and urllib.parse.quote(object_key_name) yields 'testobject%2B' respectively
        # And request_uri is
        # '/indexes/AAAAAAAAAHg%3D-AwAQAAAAAAA%3D/testobject%2B'

        request_uri = '/indexes/' + \
            urllib.parse.quote(index_id, safe='') + '/' + \
            urllib.parse.quote(object_key_name)

        body = ""
        query_params = ""
        headers = self.s3_util.prepare_signed_header('DELETE', request_uri, query_params, body)

        if(headers['Authorization'] is None):
            self._logger.error("Failed to generate v4 signature")
            return False, None

        try:
            response = super(
                CORTXS3KVApi,
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
            self._logger.info('Key value deleted.')
            return True, CORTXS3SuccessResponse(response['body'])
        else:
            self._logger.info('Failed to delete key value.')
            return False, CORTXS3ErrorResponse(
                response['status'], response['reason'], response['body'])
