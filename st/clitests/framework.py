import os
import sys
import shutil
import time
from scripttest import TestFileEnvironment

class Config:
	log_enabled = False
	dummy_run = False
	config_file = 'pathstyle.s3cfg'

def logit(str):
	if Config.log_enabled:
		print str

class PyCliTest(object):
	'Base class for all test cases.'

	def __init__(self, description):
		self.description = description
		self.command = ''
		self._create_temp_working_dir()
		self.env = TestFileEnvironment(base_path = self.working_dir)

	def _create_temp_working_dir(self):
		self.working_dir = os.path.join(os.path.dirname(os.path.realpath(__file__)), 'tests-out')

	def with_cli(self, command):
		self.command = command
		return self

	def setup(self):
		# Do some initializations required to run the tests
		logit("Setting up the test [%s]" % (self.description))
		return self

	def run(self):
		# Execute the test.
		logit("Running command: [%s]" % (self.command))
		if Config.dummy_run:
			self.status = self.env.run("echo [%s]" % (self.command))
		else:
			self.status = self.env.run(self.command)
		logit("returncode: [%d]" % (self.status.returncode))
		logit("stdout: [%s]" % (self.status.stdout))
		logit("stderr: [%s]" % (self.status.stderr))
		return self

	def teardown(self):
		# Do some cleanup
		logit("Running teardown [%s]" % (self.description))
		shutil.rmtree(self.working_dir, ignore_errors=True)
		return self

	def execute_test(self):
		print "\nTest case [%s]" % (self.description)
		self.setup()
		self.run()
		self.teardown()
		return self

	def command_is_successful(self):
		if not Config.dummy_run:
			assert self.status.returncode == 0, 'Test Failed'
		print "Command was successful."
		return self

	def command_response_should_have(self, msg):
		if not Config.dummy_run:
			assert msg in self.status.stdout
		print "Response has [%s]." % (msg)
		return self

	def command_response_should_not_have(self, msg):
		if not Config.dummy_run:
			assert msg not in self.status.stdout
		print "Response does not have [%s]." % (msg)
		return self

	def command_error_should_have(self, msg):
		if not Config.dummy_run:
			assert msg in self.status.stderr
		print "Error message has [%s]." % (msg)
		return self

	def command_error_should_not_have(self, msg):
		if not Config.dummy_run:
			assert msg in self.status.stderr
		print "Error message does not have [%s]." % (msg)
		return self

	def command_created_file(self, file_name):
		logit("Checking for file [%s] in files created [%s]" % (file_name, ', '.join(self.status.files_created)))
		if not Config.dummy_run:
			assert file_name in self.status.files_created, file_name + ' NOT created'
		print "File [%s] was created." % (file_name)
		return self

	def command_deleted_file(self, file_name):
		logit("Checking for file [%s] in files deleted [%s]" % (file_name, ', '.join(self.status.files_deleted)))
		if not Config.dummy_run:
			assert file_name in self.status.files_deleted, file_name + ' NOT deleted'
		print "File [%s] was deleted." % (file_name)
		return self

	def command_updated_file(self, file_name):
		logit("Checking for file [%s] in files updated [%s]" % (file_name, ', '.join(self.status.files_updated)))
		if not Config.dummy_run:
			assert file_name in self.status.files_updated, file_name + ' NOT updated'
		print "File [%s] was updated." % (file_name)
		return self
