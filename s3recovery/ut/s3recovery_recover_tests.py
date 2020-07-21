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

 Original author: Siddhivinayak Shanbhag  <siddhivinayak.shanbhag@seagate.com>
 Original creation date: 20-July-2020
'''
#!/usr/bin/python3.6

import mock
import unittest

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

        mockS3RecoverCorruption.check_consistency()

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

        mockS3RecoverCorruption.check_consistency()

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

        mockS3RecoverCorruption.check_consistency()

        # Assert inconsistent data should not be recovered
        self.assertEqual(len(mockS3RecoverCorruption.common_keys), 1)
        self.assertEqual(mockS3RecoverCorruption.common_keys, ["key3"])
