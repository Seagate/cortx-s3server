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

#!/usr/bin/python3.6

import mock
import unittest

from s3backgrounddelete.cortx_s3_kv_api import CORTXS3KVApi
from s3backgrounddelete.cortx_s3_success_response import CORTXS3SuccessResponse

from s3recovery.s3recovercorruption import S3RecoverCorruption
from s3recovery.s3recoverybase import S3RecoveryBase

from s3recovery.config import Config

class S3RecoverCorruptionTestCase(unittest.TestCase):

    @mock.patch.object(S3RecoveryBase, 'initiate')
    @mock.patch.object(S3RecoveryBase, 'dry_run')
    @mock.patch.object(S3RecoverCorruption, 'check_consistency')
    def test_check_consistency_check_for_recover(self,mock_initiate, mock_dry_run, mock_check_consistency):
        # Tests to check consistency check is used during recover option
        mockS3RecoverCorruption = S3RecoverCorruption()
        mock_initiate.return_value = None
        mock_dry_run.return_value = {}
        mock_check_consistency.return_value = None
        mockS3RecoverCorruption.recover_corruption("Global bucket index",
            Config.global_bucket_index_id,
            Config.global_bucket_index_id_replica,
            "Bucket metadata index",
            Config.bucket_metadata_index_id,
            Config.bucket_metadata_index_id_replica)

        self.assertTrue(mock_initiate.called)
        self.assertTrue(mock_dry_run.called)
        self.assertTrue(mock_check_consistency.called)

        #  Assert Consistency and other mock calls
        self.assertEqual(S3RecoveryBase.initiate.call_count, 2)
        self.assertEqual(S3RecoveryBase.dry_run.call_count, 2)
        self.assertEqual(S3RecoverCorruption.check_consistency.call_count, 1)

    @mock.patch.object(S3RecoveryBase, 'initiate')
    @mock.patch.object(S3RecoveryBase, 'dry_run')
    @mock.patch.object(S3RecoverCorruption, 'restore_data')
    def test_check_restore_for_recover(self,mock_initiate, mock_dry_run, mock_restore_data):
        # Tests to check restore (PutKV) is used during recover option
        mockS3RecoverCorruption = S3RecoverCorruption()
        mock_initiate.return_value = None
        mock_dry_run.return_value = {}
        mock_restore_data.return_value = None
        mockS3RecoverCorruption.recover_corruption("Global bucket index",
            Config.global_bucket_index_id,
            Config.global_bucket_index_id_replica,
            "Bucket metadata index",
            Config.bucket_metadata_index_id,
            Config.bucket_metadata_index_id_replica)

        self.assertTrue(mock_initiate.called)
        self.assertTrue(mock_dry_run.called)
        self.assertTrue(mock_restore_data.called)

        # Assert PutKV and other mock calls
        self.assertEqual(S3RecoveryBase.initiate.call_count, 2)
        self.assertEqual(S3RecoveryBase.dry_run.call_count, 2)
        self.assertEqual(S3RecoverCorruption.restore_data.call_count, 1)

    def test_inconsistent_data_entries(self):
        # Tests to check consistency check works for empty indexes
        mockS3RecoverCorruption = S3RecoverCorruption()
        mockS3RecoverCorruption.list_result = {
            "key1": "value1",
        }
        mockS3RecoverCorruption.metadata_result = {
            "617326/key2": "value2",
        }

        mockS3RecoverCorruption.check_consistency("list_index_id", "list_index_id_replica", "metadata_index_id", "metadata_index_id_replica")

        # Assert inconsistent data should not be recovered
        self.assertEqual(len(mockS3RecoverCorruption.common_keys), 0)
        self.assertEqual(mockS3RecoverCorruption.common_keys, [])

    def test_consistent_data_entries(self):
        # Tests to check consistent data is recovered during recovery
        mockS3RecoverCorruption = S3RecoverCorruption()
        mockS3RecoverCorruption.list_result = {
            "key1": "value1",
            "key2": "value2"
        }
        mockS3RecoverCorruption.metadata_result = {
            "617326/key1": "value1",
            "617326/key2": "value2"
        }

        mockS3RecoverCorruption.check_consistency("list_index_id", "list_index_id_replica", "metadata_index_id", "metadata_index_id_replica")

        # Assert for data to be recovered
        self.assertEqual(len(mockS3RecoverCorruption.common_keys), 2)
        self.assertEqual(mockS3RecoverCorruption.common_keys, ["key1","key2"])

    def test_partial_inconsistent_data_entries(self):
        # Tests to check isconsistent data is not recovered during recovery
        mockS3RecoverCorruption = S3RecoverCorruption()
        mockS3RecoverCorruption.list_result = {
            "key1": "value1",
            "key2": "value2",
            "key3": "value3"
        }
        mockS3RecoverCorruption.metadata_result = {
            "617326/key3": "value3",
            "617326/key4": "value4",
            "617326/key5": "value5"
        }

        mockS3RecoverCorruption.check_consistency("list_index_id", "list_index_id_replica", "metadata_index_id", "metadata_index_id_replica")

        # Assert inconsistent data should not be recovered
        self.assertEqual(len(mockS3RecoverCorruption.common_keys), 1)
        self.assertEqual(mockS3RecoverCorruption.common_keys, ["key3"])

    @mock.patch.object(CORTXS3KVApi, 'put')
    def test_restore_data_none_index_list(self, mock_put):
        # Test 'restore_data' when list: 'list_result' is None
        mockS3RecoverCorruption = S3RecoverCorruption()
        mockS3RecoverCorruption.list_result = None

        mockS3RecoverCorruption.restore_data('global_list_index_id',
        'replica_list_index_id',
        'global_metadata_index_id',
        'replica_metadata_index_id'
        )
        self.assertEqual(mock_put.call_count, 0)

    @mock.patch.object(CORTXS3KVApi, 'put')
    def test_restore_data_none_metadata_list(self, mock_put):
        # Test 'restore_data' when dict: 'metadata_result' is None
        mockS3RecoverCorruption = S3RecoverCorruption()
        mockS3RecoverCorruption.list_result = dict()
        mockS3RecoverCorruption.metadata_result = None

        mockS3RecoverCorruption.restore_data('global_list_index_id',
        'replica_list_index_id',
        'global_metadata_index_id',
        'replica_metadata_index_id'
        )
        self.assertEqual(mock_put.call_count, 0)

    @mock.patch.object(CORTXS3KVApi, 'put')
    def test_restore_data_empty_index_list(self, mock_put):
        # Test 'restore_data' when dict: 'list_result' is empty
        mockS3RecoverCorruption = S3RecoverCorruption()
        mockS3RecoverCorruption.list_result = dict()
        mockS3RecoverCorruption.metadata_result = dict()

        mockS3RecoverCorruption.restore_data('global_list_index_id',
        'replica_list_index_id',
        'global_metadata_index_id',
        'replica_metadata_index_id'
        )
        self.assertEqual(mock_put.call_count, 0)

    @mock.patch.object(S3RecoveryBase, 'put_kv')
    @mock.patch.object(S3RecoverCorruption, 'cleanup_bucket_list_entries')
    @mock.patch.object(S3RecoverCorruption, 'cleanup_bucket_metadata_entries')
    def test_restore_data_non_empty_index_list(self, mock_put_kv, mock_cleanup_bucket_list_entries, mock_cleanup_bucket_metadata_entries):
        # Test 'restore_data' when dict: 'list_result' & 'metadata_result' is not empty
        mockS3RecoverCorruption = S3RecoverCorruption()

        mockS3RecoverCorruption.metadata_result = {
        r'123/key3': 'value3'
        }
        mockS3RecoverCorruption.list_result = {
        'key1': 'value1',
        'key2': 'value2'
        }
        mockS3RecoverCorruption.common_keys = ['key1', 'key3']

        mock_put_kv.return_value = None
        mock_cleanup_bucket_list_entries.return_value = None
        mock_cleanup_bucket_metadata_entries.return_value = None

        mockS3RecoverCorruption.restore_data('global_list_index_id',
        'replica_list_index_id',
        'global_metadata_index_id',
        'replica_metadata_index_id')

        self.assertEqual(mock_put_kv.call_count, 2) # 1 put_kv call each to CORTXS3KVApi::put, for key1 and key3
        self.assertEqual(mock_cleanup_bucket_list_entries.call_count, 2)
        self.assertEqual(mock_cleanup_bucket_metadata_entries.call_count, 4)

