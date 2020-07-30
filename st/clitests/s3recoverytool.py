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

