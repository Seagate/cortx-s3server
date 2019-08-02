"""
Unit Test for EOSCoreClient API.
"""
#!/usr/bin/python3

from unittest.mock import Mock, patch
import http.client
import pytest

from eos_core_client import EOSCoreClient
from eos_core_config import EOSCoreConfig


def test_get_connection_success():
    """Test if HTTPConnection object is returned."""
    config = EOSCoreConfig()
    config._config['eos_core']['endpoint'] = "http://127.0.0.1:5000"
    response = EOSCoreClient(config)._get_connection()
    assert isinstance(response, http.client.HTTPConnection)


def test_get_connection_as_none():
    """
    Test if get_connection does not has endpoint configured then
    it should return "None"
    """
    config = EOSCoreConfig()
    del config._config['eos_core']['endpoint']
    assert EOSCoreClient(config)._get_connection() is None


def test_get_failure():
    """
    Test if connection object is "None" then GET method should throw TypeError.
    """
    with pytest.raises(TypeError):
        config = EOSCoreConfig()
        del config._config['eos_core']['endpoint']
        assert EOSCoreClient(config).get('/indexes/test_index1')


def test_get_success():
    """Test GET request should return success response."""
    httpconnection = Mock(spec=http.client.HTTPConnection)
    httpresponse = Mock(spec=http.client.HTTPResponse)
    httpresponse.status = 200
    httpresponse.getheaders.return_value = \
        'Content-Type:text/html;Content-Length:14'
    httpresponse.read.return_value = b'test body'
    httpresponse.reason = 'OK'
    with patch.object(http.client.HTTPConnection, 'request',
                      return_value=httpconnection):
        with patch.object(http.client.HTTPConnection, 'getresponse',
                          return_value=httpresponse):
            config = EOSCoreConfig()
            response = EOSCoreClient(config).get('/indexes/test_index1')
            assert response['status'] == 200


def test_put_failure():
    """
    Test if connection object is "None" then PUT method should throw TypeError.
    """
    with pytest.raises(TypeError):
        config = EOSCoreConfig()
        del config._config['eos_core']['endpoint']
        assert EOSCoreClient(config).put('/indexes/test_index1')


def test_put_success():
    """Test PUT request should return success response."""
    httpconnection = Mock(spec=http.client.HTTPConnection)
    httpresponse = Mock(spec=http.client.HTTPResponse)
    httpresponse.status = 201
    httpresponse.getheaders.return_value = \
        'Content-Type:text/html;Content-Length:14'
    httpresponse.read.return_value = b'test body'
    httpresponse.reason = 'OK'
    with patch.object(http.client.HTTPConnection, 'request',
                      return_value=httpconnection):
        with patch.object(http.client.HTTPConnection, 'getresponse',
                          return_value=httpresponse):
            config = EOSCoreConfig()
            request_uri = '/indexes/test_index1'
            response = EOSCoreClient(config).put(request_uri)
            assert response['status'] == 201


def test_delete_failure():
    """
    Test if connection object is "None" then DELETE should throw TypeError.
    """
    with pytest.raises(TypeError):
        config = EOSCoreConfig()
        del config._config['eos_core']['endpoint']
        assert EOSCoreClient(config).delete('/indexes/test_index1')


def test_delete_success():
    """Test DELETE request should return success response."""
    httpconnection = Mock(spec=http.client.HTTPConnection)
    httpresponse = Mock(spec=http.client.HTTPResponse)
    httpresponse.status = 204
    httpresponse.getheaders.return_value = \
        'Content-Type:text/html;Content-Length:14'
    httpresponse.read.return_value = b'test body'
    httpresponse.reason = 'OK'
    with patch.object(http.client.HTTPConnection, 'request',
                      return_value=httpconnection):
        with patch.object(http.client.HTTPConnection, 'getresponse',
                          return_value=httpresponse):
            config = EOSCoreConfig()
            response = EOSCoreClient(config).delete('/indexes/test_index1')
            assert response['status'] == 204
