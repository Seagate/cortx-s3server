
import os
from framework import PyCliTest
from framework import Config
from framework import logit

# 
class S3cmdTest(PyCliTest):
	def __init__(self, description):
		self.s3cfg = os.path.join(os.path.dirname(os.path.realpath(__file__)), 'tests.s3cfg')
		super(S3cmdTest, self).__init__(description)

	def setup(self):
		if hasattr(self, 'filename') and hasattr(self, 'filesize'):
			file_to_create = os.path.join(self.working_dir, self.filename)
			logit("Creating file [%s] with size [%d]" % (file_to_create, self.filesize))
			with open(file_to_create, 'wb') as fout:
				fout.write(os.urandom(self.filesize))
		super(S3cmdTest, self).setup()

	def run(self):
		super(S3cmdTest, self).run()

	def teardown(self):
		super(S3cmdTest, self).teardown()

	def upload_test(self, filename, filesize):
		self.filename = filename
		self.filesize = filesize
		self.with_cli("s3cmd -c " + self.s3cfg + " put " + self.filename + " s3://evault")
		return self

	def download_test(self, filename):
		self.filename = filename
		self.with_cli("s3cmd -c " + self.s3cfg + " get " + "s3://evault/" + filename)
		return self

	def delete_test(self, filename):
		self.filename = filename
		self.with_cli("s3cmd -c " + self.s3cfg + " del " + "s3://evault/" + filename)
		return self

