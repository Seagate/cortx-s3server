"""
Unit Test for EOSCoreKVAPI.
"""
#!/usr/bin/python3
from unittest.mock import patch

from eos_core_kv_api import EOSCoreKVApi
from eos_core_client import EOSCoreClient
from eos_core_config import EOSCoreConfig


def test_get_no_index():
    """Test GET api without index should return response as "None"."""
    config = EOSCoreConfig()
    response = EOSCoreKVApi(config).get(None, "test_key1")
    assert response is None


def test_get_no_key():
    """Test GET api without key should return response as "None"."""
    config = EOSCoreConfig()
    response = EOSCoreKVApi(config).get("test_index1", None)
    assert response is None


def test_get_success():
    """Test GET api, it should return success response."""
    result = {'status': 200,
              'headers': 'Content-Type:text/html;Content-Length:14',
              'body': b'{}', 'reason': 'OK'}

    with patch.object(EOSCoreClient, 'get', return_value=result):
        config = EOSCoreConfig()
        response = EOSCoreKVApi(config).get("test_index1", "test_key1")
        assert response[0] is True


def test_get_failure():
    """
    Test if index or key is not present then
    GET should return failure response.
    """
    result = {'status': 404,
              'headers': 'Content-Type:text/html;Content-Length:14',
              'body': b'{}', 'reason': 'NOT FOUND'}

    with patch.object(EOSCoreClient, 'get', return_value=result):
        config = EOSCoreConfig()
        response = EOSCoreKVApi(config).get("test_index2", "test_key2")
        assert response[0] is False


def test_delete_no_index():
    """Test DELETE api without index should return response as "None"."""
    config = EOSCoreConfig()
    response = EOSCoreKVApi(config).delete(None, "test_key1")
    assert response is None


def test_delete_no_key():
    """Test DELETE api without key should return response as "None"."""
    config = EOSCoreConfig()
    response = EOSCoreKVApi(config).delete("test_index1", None)
    assert response is None


def test_delete_success():
    """Test DELETE api, it should return success response."""
    result = {'status': 204,
              'headers': 'Content-Type:text/html;Content-Length:14',
              'body': b'{}', 'reason': 'OK'}

    with patch.object(EOSCoreClient, 'delete', return_value=result):
        config = EOSCoreConfig()
        response = EOSCoreKVApi(config).delete("test_index1", "test_key1")
        assert response[0] is True


def test_delete_failure():
    """
    Test if index or key is not present then
    DELETE should return failure response.
    """
    result = {'status': 409,
              'headers': 'Content-Type:text/html;Content-Length:14',
              'body': b'Index already exists.', 'reason': 'CONFLICT'}

    with patch.object(EOSCoreClient, 'delete', return_value=result):
        config = EOSCoreConfig()
        response = EOSCoreKVApi(config).delete("test_index1", "test_key1")
        assert response[0] is False


def test_put_no_index():
    """Test PUT api without index should return response as "None"."""
    config = EOSCoreConfig()
    response = EOSCoreKVApi(config).put(None, "test_key1")
    assert response is None


def test_put_no_key():
    """Test PUT api without key should return response as "None"."""
    config = EOSCoreConfig()
    response = EOSCoreKVApi(config).put("test_index1", None)
    assert response is None


def test_put_success():
    """Test PUT api, it should return success response."""
    result = {'status': 201,
              'headers': 'Content-Type:text/html;Content-Length:14',
              'body': b'{}', 'reason': 'OK'}

    with patch.object(EOSCoreClient, 'put', return_value=result):
        config = EOSCoreConfig()
        response = EOSCoreKVApi(config).put("test_index1", "test_value1")
        assert response[0] is True


def test_put_failure():
    """
    Test if index and key is already present then
    PUT should return failure response.
    """
    result = {'status': 400,
              'headers': 'Content-Type:text/html;Content-Length:14',
              'body': b'Index already exists.', 'reason': 'CONFLICT'}

    with patch.object(EOSCoreClient, 'put', return_value=result):
        config = EOSCoreConfig()
        response = EOSCoreKVApi(config).put("test_index2", "test_value2")
        assert response[0] is False
