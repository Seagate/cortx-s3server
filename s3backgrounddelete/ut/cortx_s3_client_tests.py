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
Unit Test for CORTXS3Client API.
"""
from http.client import HTTPConnection
from http.client import HTTPResponse
from unittest.mock import Mock
import pytest

from s3backgrounddelete.cortx_s3_client import CORTXS3Client
from s3backgrounddelete.cortx_s3_config import CORTXS3Config

def test_get_connection_success():
    """Test if HTTPConnection object is returned"""
    config = CORTXS3Config(use_cipher = False)
    response = CORTXS3Client(config)._get_connection()
    assert isinstance(response, HTTPConnection)


def test_get_connection_as_none():
    """
    Test if get_connection does not has endpoint configured then
    it should return "None"
    """
    config = Mock(spec=CORTXS3Config)
    config.get_cortx_s3_endpoint = Mock(side_effect=KeyError())
    assert CORTXS3Client(config)._get_connection() is None


def test_get_failure():
    """
    Test if connection object is "None" then GET method should throw TypeError.
    """
    with pytest.raises(TypeError):
        config = Mock(spec=CORTXS3Config)
        config.get_cortx_s3_endpoint = Mock(side_effect=KeyError())
        assert CORTXS3Client(config).get('/indexes/test_index1')


def test_get_success():
    """Test GET request should return success response."""
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
    response = CORTXS3Client(config, connection=httpconnection).get(
        '/indexes/test_index1')
    assert response['status'] == 200


def test_put_failure():
    """
    Test if connection object is "None" then PUT method should throw TypeError.
    """
    with pytest.raises(TypeError):
        config = Mock(spec=CORTXS3Config)
        config.get_cortx_s3_endpoint = Mock(side_effect=KeyError())
        assert CORTXS3Client(config).put('/indexes/test_index1')


def test_put_success():
    """Test PUT request should return success response."""
    httpconnection = Mock(spec=HTTPConnection)
    httpresponse = Mock(spec=HTTPResponse)
    httpresponse.status = 201
    httpresponse.getheaders.return_value = \
        'Content-Type:text/html;Content-Length:14'
    httpresponse.read.return_value = b'{}'
    httpresponse.reason = 'CREATED'
    httpconnection.getresponse.return_value = httpresponse

    config = CORTXS3Config(use_cipher = False)
    request_uri = '/indexes/test_index1'
    response = CORTXS3Client(config, connection=httpconnection).put(request_uri)
    assert response['status'] == 201


def test_delete_failure():
    """
    Test if connection object is "None" then DELETE should throw TypeError.
    """
    with pytest.raises(TypeError):
        config = Mock(spec=CORTXS3Config)
        config.get_cortx_s3_endpoint = Mock(side_effect=KeyError())
        assert CORTXS3Client(config).delete('/indexes/test_index1')


def test_delete_success():
    """Test DELETE request should return success response."""
    httpconnection = Mock(spec=HTTPConnection)
    httpresponse = Mock(spec=HTTPResponse)
    httpresponse.status = 204
    httpresponse.getheaders.return_value = \
        'Content-Type:text/html;Content-Length:14'
    httpresponse.read.return_value = b'test body'
    httpresponse.reason = 'OK'
    httpconnection.getresponse.return_value = httpresponse

    config = CORTXS3Config(use_cipher = False)
    response = CORTXS3Client(config, connection=httpconnection).delete(
        '/indexes/test_index1')
    assert response['status'] == 204

def test_head_failure():
    """
    Test if connection object is "None" then HEAD should throw TypeError.
    """
    with pytest.raises(TypeError):
        config = Mock(spec=CORTXS3Config)
        config.get_cortx_s3_endpoint = Mock(side_effect=KeyError())
        assert CORTXS3Client(config).head('/indexes/test_index1')


def test_head_success():
    """Test HEAD request should return success response."""
    httpconnection = Mock(spec=HTTPConnection)
    httpresponse = Mock(spec=HTTPResponse)
    httpresponse.status = 200
    httpresponse.getheaders.return_value = \
        'Content-Type:text/html;Content-Length:14'
    httpresponse.read.return_value = b'test body'
    httpresponse.reason = 'OK'
    httpconnection.getresponse.return_value = httpresponse

    config = CORTXS3Config(use_cipher = False)
    response = CORTXS3Client(config, connection=httpconnection).head(
        '/indexes/test_index1')
    assert response['status'] == 200

