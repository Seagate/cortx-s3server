"""
Unit Test for EOSCoreIndex API.
"""
#!/usr/bin/python3
from unittest.mock import patch

from eos_core_index_api import EOSCoreIndexApi
from eos_core_client import EOSCoreClient
from eos_core_config import EOSCoreConfig


def test_list_no_index():
    """Test List api without index should return response as "None"."""
    config = EOSCoreConfig()
    response = EOSCoreIndexApi(config).list(None)
    assert response is None


def test_list_success():
    """Test List api, it should return success response."""
    result = {'status': 200,
              'headers': 'Content-Type:text/html;Content-Length:14',
              'body': b'{}', 'reason': 'OK'}

    with patch.object(EOSCoreClient, 'get', return_value=result):
        config = EOSCoreConfig()
        response = EOSCoreIndexApi(config).list("test_index1")
        assert response[0] is True


def test_list_failure():
    """Test if index is not present then List should return failure response."""
    result = {'status': 404,
              'headers': 'Content-Type:text/html;Content-Length:14',
              'body': b'{}', 'reason': 'NOT FOUND'}

    with patch.object(EOSCoreClient, 'get', return_value=result):
        config = EOSCoreConfig()
        response = EOSCoreIndexApi(config).list("test_index2")
        assert response[0] is False


def test_put_success():
    """Test PUT api, it should return success response."""
    result = {'status': 201,
              'headers': 'Content-Type:text/html;Content-Length:14',
              'body': b'{}', 'reason': 'OK'}

    with patch.object(EOSCoreClient, 'put', return_value=result):
        config = EOSCoreConfig()
        response = EOSCoreIndexApi(config).put("test_index1")
        assert response[0] is True


def test_put_failure():
    """Test if index is already present then PUT should return failure response."""
    result = {'status': 409,
              'headers': 'Content-Type:text/html;Content-Length:14',
              'body': b'Index already exists.', 'reason': 'CONFLICT'}

    with patch.object(EOSCoreClient, 'put', return_value=result):
        config = EOSCoreConfig()
        response = EOSCoreIndexApi(config).put("test_index1")
        assert response[0] is False


def test_put_no_index():
    """Test PUT request without index, it should return response as "None"."""
    config = EOSCoreConfig()
    response = EOSCoreIndexApi(config).put(None)
    assert response is None
