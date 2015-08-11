#!/usr/bin/python

from framework import Config
from s3cmd import S3cmdTest

# Helps debugging
# Config.log_enabled = True
# Config.dummy_run = True

#  ***MAIN ENTRY POINT

# ************ 3k FILE TEST ************
S3cmdTest('s3cmd can upload 3k file').upload_test("3kfile", 3000).execute_test().command_is_successful()

S3cmdTest('s3cmd can download 3k file').download_test("3kfile").execute_test().command_is_successful().command_created_file("3kfile")

S3cmdTest('s3cmd can delete 3k file').delete_test("3kfile").execute_test().command_is_successful()

# ************ 8k FILE TEST ************
S3cmdTest('s3cmd can upload 8k file').upload_test("8kfile", 8192).execute_test().command_is_successful()

S3cmdTest('s3cmd can download 8k file').download_test("8kfile").execute_test().command_is_successful().command_created_file("8kfile")

S3cmdTest('s3cmd can delete 8k file').delete_test("8kfile").execute_test().command_is_successful()

# ************ 700k FILE TEST ************
S3cmdTest('s3cmd can upload 700k file').upload_test("700kfile", 716800).execute_test().command_is_successful()

S3cmdTest('s3cmd can download 700k file').download_test("700kfile").execute_test().command_is_successful().command_created_file("700kfile")

S3cmdTest('s3cmd can delete 700k file').delete_test("700kfile").execute_test().command_is_successful()