"""
Unit Test for EOSCoreObject API.
"""
#!/usr/bin/python3

from unittest.mock import patch

from eos_core_object_api import EOSCoreObjectApi
from eos_core_client import EOSCoreClient
from eos_core_config import EOSCoreConfig


def test_get_no_oid():
    """Test GET api without oid should return response as "None"."""
    config = EOSCoreConfig()
    response = EOSCoreObjectApi(config).get(None)
    assert response is None


def test_get_success():
    """Test GET api, it should return success response."""
    result = {'status': 200,
              'headers': 'Content-Type:text/html;Content-Length:14',
              'body': b'{}', 'reason': 'OK'}

    with patch.object(EOSCoreClient, 'get', return_value=result):
        config = EOSCoreConfig()
        response = EOSCoreObjectApi(config).get("test_object1")
        assert response[0] is True


def test_get_failure():
    """Test if GET api fails it should return error response"""
    result = {'status': 404,
              'headers': 'Content-Type:text/html;Content-Length:14',
              'body': b'{}', 'reason': 'NOT FOUND'}

    with patch.object(EOSCoreClient, 'get', return_value=result):
        config = EOSCoreConfig()
        response = EOSCoreObjectApi(config).get("test_index2")
        assert response[0] is False


def test_put_success():
    """Test PUT api, it should send success response."""
    result = {'status': 201,
              'headers': 'Content-Type:text/html;Content-Length:14',
              'body': b'{}', 'reason': 'OK'}

    with patch.object(EOSCoreClient, 'put', return_value=result):
        config = EOSCoreConfig()
        response = EOSCoreObjectApi(config).put("test_index1", "test_value1")
        assert response[0] is True


def test_put_failure():
    """Test if PUT fails it should return error response."""
    result = {'status': 409,
              'headers': 'Content-Type:text/html;Content-Length:14',
              'body': b'Index already exists.', 'reason': 'CONFLICT'}

    with patch.object(EOSCoreClient, 'put', return_value=result):
        config = EOSCoreConfig()
        response = EOSCoreObjectApi(config).put("test_index2", "test_value2")
        assert response[0] is False


def test_put_no_index():
    """Test PUT api with no index should return response as "None"."""
    config = EOSCoreConfig()
    response = EOSCoreObjectApi(config).put(None, "test_value2")
    assert response is None


def test_delete_success():
    """Test DELETE api, it should send success response."""
    result = {'status': 204,
              'headers': 'Content-Type:text/html;Content-Length:14',
              'body': b'{}', 'reason': 'OK'}

    with patch.object(EOSCoreClient, 'delete', return_value=result):
        config = EOSCoreConfig()
        response = EOSCoreObjectApi(config).delete("test_index1")
        assert response[0] is True


def test_delete_failure():
    """Test if DELETE api fails, it should return error response."""
    result = {'status': 409,
              'headers': 'Content-Type:text/html;Content-Length:14',
              'body': b'Index already exists.', 'reason': 'CONFLICT'}

    with patch.object(EOSCoreClient, 'delete', return_value=result):
        config = EOSCoreConfig()
        response = EOSCoreObjectApi(config).delete("test_index1")
        assert response[0] is False


def test_delete_no_index():
    """Test DELETE api with no index, it should return response as "None"."""
    config = EOSCoreConfig()
    response = EOSCoreObjectApi(config).delete(None)
    assert response is None
