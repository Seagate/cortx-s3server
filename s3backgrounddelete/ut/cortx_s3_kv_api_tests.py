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

"""
Unit Test for CORTXS3KVAPI.
"""
from http.client import HTTPConnection
from http.client import HTTPResponse
from unittest.mock import Mock

from s3backgrounddelete.cortx_s3_kv_api import CORTXS3KVApi
from s3backgrounddelete.cortx_s3_config import CORTXS3Config

def test_get_no_index_id():
    """Test GET api without index_id should return response as "None"."""
    config = CORTXS3Config(use_cipher = False)
    response = CORTXS3KVApi(config).get(None, "test_key1")
    if (response is not None):
        assert response[0] is False
        assert response[1] is None


def test_get_no_object_key_name():
    """Test GET api without object_key_name should return response as "None"."""
    config = CORTXS3Config(use_cipher = False)
    response = CORTXS3KVApi(config).get("test_index1", None)
    if (response is not None):
        assert response[0] is False


def test_get_success():
    """Test GET api, it should return success response."""
    result = b'{"Key": "test_key1", "Value": "testValue1"}'
    httpconnection = Mock(spec=HTTPConnection)
    httpresponse = Mock(spec=HTTPResponse)
    httpresponse.status = 200
    httpresponse.getheaders.return_value = \
        'Content-Type:text/html;Content-Length:14'
    httpresponse.read.return_value = result
    httpresponse.reason = 'OK'
    httpconnection.getresponse.return_value = httpresponse

    config = CORTXS3Config(use_cipher = False)
    response = CORTXS3KVApi(config, connection=httpconnection).get("test_index1", "test_key1")
    if (response is not None):
        assert response[0] is True


def test_get_failure():
    """
    Test if index or key is not present then
    GET should return failure response.
    """
    httpconnection = Mock(spec=HTTPConnection)
    httpresponse = Mock(spec=HTTPResponse)
    httpresponse.status = 404
    httpresponse.getheaders.return_value = \
        'Content-Type:text/html;Content-Length:14'
    httpresponse.read.return_value = b'{}'
    httpresponse.reason = 'NOT FOUND'
    httpconnection.getresponse.return_value = httpresponse

    config = CORTXS3Config(use_cipher = False)
    response = CORTXS3KVApi(config, connection=httpconnection).get("test_index2", "test_key2")
    if (response is not None):
        assert response[0] is False


def test_delete_no_index_id():
    """Test DELETE api without index should return response as "None"."""
    config = CORTXS3Config(use_cipher = False)
    response = CORTXS3KVApi(config).delete(None, "test_key1")
    if (response is not None):
        assert response[0] is False
        assert response[1] is None


def test_delete_no_object_key_name():
    """Test DELETE api without object key name should return response as "None"."""
    config = CORTXS3Config(use_cipher = False)
    response = CORTXS3KVApi(config).delete("test_index1", None)
    if (response is not None):
        assert response[0] is False
        assert response[1] is None


def test_delete_success():
    """Test DELETE api, it should return success response."""
    httpconnection = Mock(spec=HTTPConnection)
    httpresponse = Mock(spec=HTTPResponse)
    httpresponse.status = 204
    httpresponse.getheaders.return_value = \
        'Content-Type:text/html;Content-Length:14'
    httpresponse.read.return_value = b'{}'
    httpresponse.reason = 'NO CONTENT'
    httpconnection.getresponse.return_value = httpresponse

    config = CORTXS3Config(use_cipher = False)
    response = CORTXS3KVApi(config, connection=httpconnection).delete("test_index1", "test_key1")
    if (response is not None):
        assert response[0] is True


def test_delete_failure():
    """
    Test if index or key is not present then
    DELETE should return failure response.
    """
    httpconnection = Mock(spec=HTTPConnection)
    httpresponse = Mock(spec=HTTPResponse)
    httpresponse.status = 404
    httpresponse.getheaders.return_value = \
        'Content-Type:text/html;Content-Length:14'
    httpresponse.read.return_value = b'{}'
    httpresponse.reason = 'NO CONTENT'
    httpconnection.getresponse.return_value = httpresponse

    config = CORTXS3Config(use_cipher = False)
    response = CORTXS3KVApi(config, connection=httpconnection).delete("test_index1", "test_key1")
    if (response is not None):
        assert response[0] is False


def test_put_no_index_id():
    """Test PUT api without index_id should return response as "None"."""
    config = CORTXS3Config(use_cipher = False)
    response = CORTXS3KVApi(config).put(None, "test_key1")
    if (response is not None):
        assert response[0] is False
        assert response[1] is None

def test_put_no_object_key_name():
    """Test PUT api without key should return response as "None"."""
    config = CORTXS3Config(use_cipher = False)
    response = CORTXS3KVApi(config).put("test_index1", None)
    if (response is not None):
        assert response[0] is False
        assert response[1] is None


def test_put_success():
    """Test PUT api, it should return success response."""
    httpconnection = Mock(spec=HTTPConnection)
    httpresponse = Mock(spec=HTTPResponse)
    httpresponse.status = 200
    httpresponse.getheaders.return_value = \
        'Content-Type:text/html;Content-Length:14'
    httpresponse.read.return_value = b'{}'
    httpresponse.reason = 'CREATED'
    httpconnection.getresponse.return_value = httpresponse

    config = CORTXS3Config(use_cipher = False)
    response = CORTXS3KVApi(config, connection=httpconnection).put("test_index1", "test_key1")
    if (response is not None):
        assert response[0] is True


def test_put_failure():
    """
    Test if index_id and object key name is already present then
    PUT should return failure response.
    """
    httpconnection = Mock(spec=HTTPConnection)
    httpresponse = Mock(spec=HTTPResponse)
    httpresponse.status = 409
    httpresponse.getheaders.return_value = \
        'Content-Type:text/html;Content-Length:14'
    httpresponse.read.return_value = b'{}'
    httpresponse.reason = 'CONFLICT'
    httpconnection.getresponse.return_value = httpresponse

    config = CORTXS3Config(use_cipher = False)
    response = CORTXS3KVApi(config, connection=httpconnection).put("test_index2", "test_key2")
    if (response is not None):
        assert response[0] is False
