#!/usr/bin/python3.6
'''
 COPYRIGHT 2020 SEAGATE LLC

 THIS DRAWING/DOCUMENT, ITS SPECIFICATIONS, AND THE DATA CONTAINED
 HEREIN, ARE THE EXCLUSIVE PROPERTY OF SEAGATE TECHNOLOGY
 LIMITED, ISSUED IN STRICT CONFIDENCE AND SHALL NOT, WITHOUT
 THE PRIOR WRITTEN PERMISSION OF SEAGATE TECHNOLOGY LIMITED,
 BE REPRODUCED, COPIED, OR DISCLOSED TO A THIRD PARTY, OR
 USED FOR ANY PURPOSE WHATSOEVER, OR STORED IN A RETRIEVAL SYSTEM
 EXCEPT AS ALLOWED BY THE TERMS OF SEAGATE LICENSES AND AGREEMENTS.

 YOU SHOULD HAVE RECEIVED A COPY OF SEAGATE'S LICENSE ALONG WITH
 THIS RELEASE. IF NOT PLEASE CONTACT A SEAGATE REPRESENTATIVE
 http://www.seagate.com/contact

 Original author: Amit Kumar  <amit.kumar@seagate.com>
 Original creation date: 08-July-2020
'''

import mock
import unittest

from s3recovery.s3recoverybase import S3RecoveryBase

from s3backgrounddelete.eos_core_config import EOSCoreConfig
from s3backgrounddelete.eos_core_index_api import EOSCoreIndexApi
from s3backgrounddelete.eos_core_kv_api import EOSCoreKVApi
from s3backgrounddelete.eos_list_index_response import EOSCoreListIndexResponse

class S3RecoveryBaseTestCase(unittest.TestCase):

    @mock.patch.object(EOSCoreIndexApi, 'list')
    def test_list_index_empty(self, mock_list):
        # Tests 'list_index' when index is empty
        mockS3RecoveryBase = S3RecoveryBase()

        # setup mock
        mock_res = '{"Delimiter":"","Index-Id":"mock-index-id","IsTruncated":"false","Keys":null,"Marker":"","MaxKeys":"1000","NextMarker":"","Prefix":""}'
        mock_list.return_value = True, EOSCoreListIndexResponse(mock_res.encode())

        keys = mockS3RecoveryBase.list_index('global_index_id')

        self.assertEqual(keys, None)

    @mock.patch.object(EOSCoreIndexApi, 'list')
    @mock.patch.object(EOSCoreListIndexResponse, 'get_index_content')
    def test_list_index_not_empty(self, mock_get_index_content, mock_list):
        # Tests 'list_index' when index is not empty
        mockS3RecoveryBase = S3RecoveryBase()

        # setup mock
        mock_json_res = '{}'
        mock_list.return_value = True, EOSCoreListIndexResponse(mock_json_res.encode("utf-8"))
        mock_get_index_content.return_value = {"Delimiter":"","Index-Id":"mock-index-id","IsTruncated":"false","Keys":[{"Key":"key-1","Value":"value-1"},{"Key":"key-2","Value":"value-2"}],"Marker":"","MaxKeys":"1000","NextMarker":"","Prefix":""}

        keys_list = mockS3RecoveryBase.list_index('global_index_id')

        self.assertNotEqual(keys_list, None)
        self.assertEqual(len(keys_list), 2)

    @mock.patch.object(EOSCoreIndexApi, 'list')
    def test_list_index_failed(self, mock_list):
        # Tests 'list_index' when EOSCoreIndexApi.list returned False
        mockS3RecoveryBase = S3RecoveryBase()

        # setup mock
        mock_res = '{}'
        mock_list.return_value = False, EOSCoreListIndexResponse(mock_res.encode())

        with self.assertRaises(SystemExit) as cm:
            mockS3RecoveryBase.list_index('global_index_id')

        self.assertEqual(cm.exception.code, 1)

    @mock.patch.object(EOSCoreIndexApi, 'list')
    @mock.patch.object(EOSCoreListIndexResponse, 'get_index_content')
    def test_initiate_list_called(self, mock_get_index_content, mock_list):
        # Test 'initiate' function for EOSCoreIndexApi.list called or not
        mockS3RecoveryBase = S3RecoveryBase()

        # setup mock
        mock_res = '{}'
        mock_list.return_value = True, EOSCoreListIndexResponse(mock_res.encode())
        mock_get_index_content.return_value = {"Delimiter":"","Index-Id":"mock-index-id","IsTruncated":"false","Keys":[{"Key":"key-1","Value":"value-1"}],"Marker":"","MaxKeys":"1000","NextMarker":"","Prefix":""}
        mockS3RecoveryBase.initiate('global_index', 'global_index_id', 'global_index_id_replica')
        
        self.assertEqual(mock_list.call_count, 2)

