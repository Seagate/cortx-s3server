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
Unit Test for CORTXS3Object API.
"""
from http.client import HTTPConnection
from http.client import HTTPResponse
from unittest.mock import Mock

from s3backgrounddelete.cortx_s3_object_api import CORTXS3ObjectApi
from s3backgrounddelete.cortx_s3_config import CORTXS3Config

def test_get_no_oid():
    """Test GET api without oid should return response as "None"."""
    config = CORTXS3Config(use_cipher = False)
    response = CORTXS3ObjectApi(config).get(None)
    if (response is not None):
        assert response[0] is False
        assert response[1] is None

def test_get_success():
    """Test GET api, it should return success response."""
    result = b'{"test_obj1": "{ \"obj-name\" : \"test_bucket_1/test_obj1\"}"}'

    httpconnection = Mock(spec=HTTPConnection)
    httpresponse = Mock(spec=HTTPResponse)
    httpresponse.status = 200
    httpresponse.getheaders.return_value = \
        'Content-Type:text/html;Content-Length:14'
    httpresponse.read.return_value = result
    httpresponse.reason = 'OK'
    httpconnection.getresponse.return_value = httpresponse

    config = CORTXS3Config(use_cipher = False)
    response = CORTXS3ObjectApi(config, connection=httpconnection).get("test_object1")
    if (response is not None):
        assert response[0] is True

def test_get_failure():
    """Test if GET api fails it should return error response"""
    httpconnection = Mock(spec=HTTPConnection)
    httpresponse = Mock(spec=HTTPResponse)
    httpresponse.status = 404
    httpresponse.getheaders.return_value = \
        'Content-Type:text/html;Content-Length:14'
    httpresponse.read.return_value = b'{}'
    httpresponse.reason = 'NOT FOUND'
    httpconnection.getresponse.return_value = httpresponse

    config = CORTXS3Config(use_cipher = False)
    response = CORTXS3ObjectApi(config, connection=httpconnection).get("test_index2")
    if (response is not None):
        assert response[0] is False

def test_put_success():
    """Test PUT api, it should send success response."""
    httpconnection = Mock(spec=HTTPConnection)
    httpresponse = Mock(spec=HTTPResponse)
    httpresponse.status = 201
    httpresponse.getheaders.return_value = \
        'Content-Type:text/html;Content-Length:14'
    httpresponse.read.return_value = b'{}'
    httpresponse.reason = 'CREATED'
    httpconnection.getresponse.return_value = httpresponse

    config = CORTXS3Config(use_cipher = False)
    response = CORTXS3ObjectApi(config, connection=httpconnection).put("test_index1", "test_value1")
    if (response is not None):
        assert response[0] is True

def test_put_failure():
    """Test if PUT fails it should return error response."""
    httpconnection = Mock(spec=HTTPConnection)
    httpresponse = Mock(spec=HTTPResponse)
    httpresponse.status = 409
    httpresponse.getheaders.return_value = \
        'Content-Type:text/html;Content-Length:14'
    httpresponse.read.return_value = b'{}'
    httpresponse.reason = 'CONFLICT'
    httpconnection.getresponse.return_value = httpresponse

    config = CORTXS3Config(use_cipher = False)
    response = CORTXS3ObjectApi(config, connection=httpconnection).put("test_index2", "test_value2")
    if (response is not None):
        assert response[0] is False

def test_put_no_index():
    """Test PUT api with no index should return response as "None"."""
    config = CORTXS3Config(use_cipher = False)
    response = CORTXS3ObjectApi(config).put(None, "test_value2")
    if (response is not None):
        assert response[0] is False

def test_delete_success():
    """Test DELETE api, it should send success response."""
    httpconnection = Mock(spec=HTTPConnection)
    httpresponse = Mock(spec=HTTPResponse)
    httpresponse.status = 204
    httpresponse.getheaders.return_value = \
        'Content-Type:text/html;Content-Length:14'
    httpresponse.read.return_value = b'{}'
    httpresponse.reason = 'NO CONTENT'
    httpconnection.getresponse.return_value = httpresponse

    config = CORTXS3Config(use_cipher = False)
    response = CORTXS3ObjectApi(config, connection=httpconnection).delete("test_oid1", "test_layout_id1")
    if (response is not None):
        assert response[0] is True

def test_delete_failure():
    """Test if DELETE api fails, it should return error response."""
    httpconnection = Mock(spec=HTTPConnection)
    httpresponse = Mock(spec=HTTPResponse)
    httpresponse.status = 404
    httpresponse.getheaders.return_value = \
        'Content-Type:text/html;Content-Length:14'
    httpresponse.read.return_value = b'{}'
    httpresponse.reason = 'NOT FOUND'
    httpconnection.getresponse.return_value = httpresponse

    config = CORTXS3Config(use_cipher = False)
    response = CORTXS3ObjectApi(config, connection=httpconnection).delete("test_oid2", "test_layout_id2")
    if (response is not None):
        assert response[0] is False

def test_delete_no_oid():
    """Test DELETE api without index, it should return response as "None"."""
    config = CORTXS3Config(use_cipher = False)
    response = CORTXS3ObjectApi(config).delete(None, "test_layot_id2")
    if (response is not None):
        assert response[0] is False
        assert response[1] is None

def test_delete_no_layout_id():
    """Test DELETE api without layout id, it should return response as "None"."""
    config = CORTXS3Config(use_cipher = False)
    response = CORTXS3ObjectApi(config).delete("test_oid1", None)
    if (response is not None):
        assert response[0] is False
        assert response[1] is None

def test_head_success():
    """Test HEAD api, it should send success response."""
    httpconnection = Mock(spec=HTTPConnection)
    httpresponse = Mock(spec=HTTPResponse)
    httpresponse.status = 200
    httpresponse.getheaders.return_value = \
        'Content-Type:text/html;Content-Length:14'
    httpresponse.read.return_value = b'{}'
    httpresponse.reason = ''
    httpconnection.getresponse.return_value = httpresponse

    config = CORTXS3Config(use_cipher = False)
    response = CORTXS3ObjectApi(config, connection=httpconnection).head("test_oid1", "test_layout_id1")
    if (response is not None):
        assert response[0] is True

def test_head_failure():
    """Test if HEAD api fails, it should return error response."""
    httpconnection = Mock(spec=HTTPConnection)
    httpresponse = Mock(spec=HTTPResponse)
    httpresponse.status = 404
    httpresponse.getheaders.return_value = \
        'Content-Type:text/html;Content-Length:14'
    httpresponse.read.return_value = b'{}'
    httpresponse.reason = 'NOT FOUND'
    httpconnection.getresponse.return_value = httpresponse

    config = CORTXS3Config(use_cipher = False)
    response = CORTXS3ObjectApi(config, connection=httpconnection).head("test_oid2", "test_layout_id2")
    if (response is not None):
        assert response[0] is False

def test_head_no_oid():
    """Test HEAD api without index, it should return response as "None"."""
    config = CORTXS3Config(use_cipher = False)
    response = CORTXS3ObjectApi(config).head(None, "test_layot_id2")
    if (response is not None):
        assert response[0] is False
        assert response[1] is None

def test_head_no_layout_id():
    """Test HEAD api without layout id, it should return response as "None"."""
    config = CORTXS3Config(use_cipher = False)
    response = CORTXS3ObjectApi(config).head("test_oid1", None)
    if (response is not None):
        assert response[0] is False
        assert response[1] is None

