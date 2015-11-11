
import os
from framework import PyCliTest
from framework import Config
from framework import logit

#
class S3cmdTest(PyCliTest):
	def __init__(self, description):
		self.s3cfg = os.path.join(os.path.dirname(os.path.realpath(__file__)), Config.config_file)
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

	def create_bucket(self, bucket_name):
		self.bucket_name = bucket_name
		self.with_cli("s3cmd -c " + self.s3cfg + " mb " + " s3://" + self.bucket_name)
		return self

	def list_buckets(self):
		self.with_cli("s3cmd -c " + self.s3cfg + " ls ")
		return self

	def delete_bucket(self, bucket_name):
		self.bucket_name = bucket_name
		self.with_cli("s3cmd -c " + self.s3cfg + " rb " + " s3://" + self.bucket_name)
		return self

	def list_objects(self, bucket_name):
		self.bucket_name = bucket_name
		self.with_cli("s3cmd -c " + self.s3cfg + " ls " + " s3://" + self.bucket_name)
		return self

	def list_specific_objects(self, bucket_name, object_pattern):
		self.bucket_name = bucket_name
		self.object_pattern = object_pattern
		self.with_cli("s3cmd -c " + self.s3cfg + " ls " + " s3://" + self.bucket_name + "/" + self.object_pattern)
		return self

	def upload_test(self, bucket_name, filename, filesize):
		self.filename = filename
		self.filesize = filesize
		self.bucket_name = bucket_name
		self.with_cli("s3cmd -c " + self.s3cfg + " put " + self.filename + " s3://" + self.bucket_name)
		return self

	def download_test(self, bucket_name, filename):
		self.filename = filename
		self.bucket_name = bucket_name
		self.with_cli("s3cmd -c " + self.s3cfg + " get " + "s3://" + self.bucket_name + "/" + self.filename)
		return self

	def delete_test(self, bucket_name, filename):
		self.filename = filename
		self.bucket_name = bucket_name
		self.with_cli("s3cmd -c " + self.s3cfg + " del " + "s3://" + self.bucket_name + "/" + self.filename)
		return self
