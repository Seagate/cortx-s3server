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
 Original creation date: 02-July-2020
'''

import os
from framework import S3PyCliTest

class S3RecoveryTest(S3PyCliTest):
  def __init__(self, description):
    file_path = os.path.dirname(os.path.realpath(__file__))
    self.cmd = file_path + r"/../../s3recovery/s3recovery/s3recovery"
    super(S3RecoveryTest, self).__init__(description)

  def s3recovery_help(self):
    helpcmd = self.cmd + " --help"
    self.with_cli(helpcmd)
    return self

  def s3recovery_dry_run(self):
    dry_run_cmd = self.cmd + " --dry_run"
    self.with_cli(dry_run_cmd)
    return self

  def s3recovery_recover(self):
    recover_cmd = self.cmd + " --recover"
    self.with_cli(recover_cmd)
    return self

