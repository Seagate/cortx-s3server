"""
Unit Test for ObjectRecoveryValidator.
"""
import logging
import json
from unittest.mock import Mock
import pytest

from s3backgrounddelete.eos_core_config import EOSCoreConfig
from s3backgrounddelete.eos_core_index_api import EOSCoreIndexApi
from s3backgrounddelete.eos_core_kv_api import EOSCoreKVApi
from s3backgrounddelete.eos_core_object_api import EOSCoreObjectApi
from s3backgrounddelete.eos_core_error_respose import EOSCoreErrorResponse
from s3backgrounddelete.eos_get_kv_response import EOSCoreGetKVResponse
from s3backgrounddelete.eos_list_index_response import EOSCoreListIndexResponse
from s3backgrounddelete.object_recovery_validator import ObjectRecoveryValidator


def test_list_instance_index_fail():
    """Test if ObjectRecoveryValidator skips processing successfully if listing instance index fails"""
    index_api_mock = Mock(spec=EOSCoreIndexApi)
    object_api_mock = Mock(spec=EOSCoreObjectApi)

    index_api_mock.list.return_value = False, {}

    config = EOSCoreConfig()
    probable_delete_records = {'Key': 'TAcGAQAAAAA=-AwAAAAAAhEs=',
                     'Value': '{"global_instance_id":"TAcGAQAAAAA=-AAAAAAAAhEs=", \
                     "index_id":"TAcGAQAAAHg=-AQAAAAAAhEs=","object_layout_id":9, \
                     "object_metadata_path":"object_1"}\n'}
    validator = ObjectRecoveryValidator(
                          config, probable_delete_records, objectapi = object_api_mock, indexapi = index_api_mock)
    validator.process_results()

    # Assert that object oid delete is skipped if instance id listing fails.
    object_api_mock.assert_not_called


def test_object_metadata_not_exists():
    """Test to check if ObjectRecoveryValidator attempts delete for object oid """
    index_api_mock = Mock(spec=EOSCoreIndexApi)
    kv_api_mock = Mock(spec=EOSCoreKVApi)
    object_api_mock = Mock(spec=EOSCoreObjectApi)

    index_content = {'Delimiter': '', 'Index-Id': 'AAAAAAAAAHg=-BAAQAAAAAAA=',
                    'IsTruncated': 'false', 'Keys': [{'Key': '<0x7200000000000000:0>', 'Value': 'TAchAQAAAAA=-AAAAAAAAJlM='}], \
                    'Marker': '', 'MaxKeys': '1000', 'NextMarker': '', 'Prefix': ''}
    index_response = EOSCoreListIndexResponse(json.dumps(index_content).encode())
    error_response = EOSCoreErrorResponse(404, "Not found", "Not found")

    index_api_mock.list.return_value = True, index_response
    kv_api_mock.get.return_value = False, error_response
    object_api_mock.delete.return_value = True, {}


    config = EOSCoreConfig()
    probable_delete_records = {'Key': 'TAcGAQAAAAA=-AwAAAAAAhEs=',
                     'Value': '{"global_instance_id":"TAcGAQAAAAA=-AAAAAAAAhEs=", \
                     "index_id":"TAcGAQAAAHg=-AQAAAAAAhEs=","object_layout_id":9, \
                     "object_metadata_path":"object_1"}\n'}
    validator = ObjectRecoveryValidator(
                          config, probable_delete_records, objectapi = object_api_mock, kvapi = kv_api_mock, indexapi = index_api_mock)
    validator.process_results()

    # Assert that object oid delete is called if metadata doesn't exists.
    object_api_mock.assert_called_once


def test_object_metadata_exists_and_matches():
    """Test if ObjectRecoveryValidator should attempt delete for object oid """
    index_api_mock = Mock(spec=EOSCoreIndexApi)
    kv_api_mock = Mock(spec=EOSCoreKVApi)
    object_api_mock = Mock(spec=EOSCoreObjectApi)

    index_content = {'Delimiter': '', 'Index-Id': 'AAAAAAAAAHg=-BAAQAAAAAAA=',
                    'IsTruncated': 'false', 'Keys': [{'Key': '<0x7200000000000000:0>', 'Value': 'TAchAQAAAAA=-AAAAAAAAJlM='}], \
                    'Marker': '', 'MaxKeys': '1000', 'NextMarker': '', 'Prefix': ''}
    index_response = EOSCoreListIndexResponse(json.dumps(index_content).encode())
    object_key = '<0x7200000000000000:0>'
    # oid mismatches in object metadata
    object_metadata = {"Object-Name":"bucket_1/obj_1", \
                           "x-amz-meta-s3cmd": "true", \
                           "mero_oid" : "DNchAQAAAAA=-AAAAAAAAJlM="}

    kv_response = EOSCoreGetKVResponse(object_key, json.dumps(object_metadata).encode())

    index_api_mock.list.return_value = True, index_response
    kv_api_mock.get.return_value = True, kv_response
    object_api_mock.delete.return_value = True, {}


    config = EOSCoreConfig()
    probable_delete_records = {'Key': 'TAcGAQAAAAA=-AwAAAAAAhEs=',
                     'Value': '{"global_instance_id":"TAcGAQAAAAA=-AAAAAAAAhEs=", \
                     "index_id":"TAcGAQAAAHg=-AQAAAAAAhEs=","object_layout_id":9, \
                     "object_metadata_path":"object_1"}\n'}
    validator = ObjectRecoveryValidator(
                          config, probable_delete_records, objectapi = object_api_mock, kvapi = kv_api_mock, indexapi = index_api_mock)
    validator.process_results()

    # Assert that object oid delete is not called if metadata exists and matches entries in probable delete index.
    object_api_mock.assert_not_called

def test_object_metadata_exists_mismatches():
    """Test if ObjectRecoveryValidator should attempt delete for object oid """
    index_api_mock = Mock(spec=EOSCoreIndexApi)
    kv_api_mock = Mock(spec=EOSCoreKVApi)
    object_api_mock = Mock(spec=EOSCoreObjectApi)

    index_content = {'Delimiter': '', 'Index-Id': 'AAAAAAAAAHg=-BAAQAAAAAAA=',
                    'IsTruncated': 'false', 'Keys': [{'Key': '<0x7200000000000000:0>', 'Value': 'TAchAQAAAAA=-AAAAAAAAJlM='}], \
                    'Marker': '', 'MaxKeys': '1000', 'NextMarker': '', 'Prefix': ''}
    index_response = EOSCoreListIndexResponse(json.dumps(index_content).encode())
    object_key = '<0x7200000000000000:0>'
    object_metadata = {"Object-Name":"bucket_1/obj_1", \
                           "x-amz-meta-s3cmd": "true", \
                           "mero_oid" : "TAchAQAAAAA=-AAAAAAAAJlM="}

    kv_response = EOSCoreGetKVResponse(object_key, json.dumps(object_metadata).encode())

    index_api_mock.list.return_value = True, index_response
    kv_api_mock.get.return_value = True, kv_response
    object_api_mock.delete.return_value = True, {}


    config = EOSCoreConfig()
    probable_delete_records = {'Key': 'TAcGAQAAAAA=-AwAAAAAAhEs=',
                     'Value': '{"global_instance_id":"TAcGAQAAAAA=-AAAAAAAAhEs=", \
                     "index_id":"TAcGAQAAAHg=-AQAAAAAAhEs=","object_layout_id":9, \
                     "object_metadata_path":"object_1"}\n'}
    validator = ObjectRecoveryValidator(
                          config, probable_delete_records, objectapi = object_api_mock, kvapi = kv_api_mock, indexapi = index_api_mock)
    validator.process_results()

    # Assert that object oid delete is called once metadata exists and matches entries in probable delete index.
    object_api_mock.assert_called_once
