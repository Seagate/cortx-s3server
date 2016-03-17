
import os
import sys
import time
from threading import Timer
import subprocess
from framework import PyCliTest
from framework import Config
from framework import logit

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

    def create_bucket(self, bucket_name, region=None):
        self.bucket_name = bucket_name
        if region:
            self.with_cli("s3cmd -c " + self.s3cfg + " mb " + " s3://" + self.bucket_name + " --bucket-location=" + region)
        else:
            self.with_cli("s3cmd -c " + self.s3cfg + " mb " + " s3://" + self.bucket_name)
        return self

    def list_buckets(self):
        self.with_cli("s3cmd -c " + self.s3cfg + " ls ")
        return self

    def info_bucket(self, bucket_name):
        self.bucket_name = bucket_name
        self.with_cli("s3cmd -c " + self.s3cfg + " info " + " s3://" + self.bucket_name)
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

    def list_multipart(self):
        my_bucket = "s3://" + self.bucket_name
        my_object = "s3://" + self.bucket_name + "/" + self.filename
        popenobj = subprocess.Popen(["s3cmd", "-c",self.s3cfg,"multipart",my_bucket], stdout=subprocess.PIPE, stderr= subprocess.PIPE)
        while not popenobj.poll():
           data = popenobj.stdout.readline()
           stdoutdata = data.decode('utf-8')
           if stdoutdata:
             objecturl = " " + my_object + " "
             if( stdoutdata.find(my_object) >= 0):
               sys.stdout.write(stdoutdata)
               upload_id = stdoutdata[-37:-1]
           else:
             break
        return self

    def abort_multipart(self):
        my_bucket = "s3://" + self.bucket_name
        my_object = "s3://" + self.bucket_name + "/" + self.filename
        popenobj = subprocess.Popen(["s3cmd", "-c",self.s3cfg,"multipart",my_bucket], stdout=subprocess.PIPE, stderr= subprocess.PIPE)
        total_output = ""
        upload_id = ""
        while not popenobj.poll():
           data = popenobj.stdout.readline()
           stdoutdata = data.decode('utf-8')
           if stdoutdata:
             if( stdoutdata.find(my_object) >= 0):
               upload_id = stdoutdata[-37:-1]
               sys.stdout.write(stdoutdata)
               break
           else:
             break

        if upload_id:
          popenabort = subprocess.Popen(["s3cmd", "-c",self.s3cfg,"abortmp",my_object, upload_id],
                                       stdout=subprocess.PIPE, stderr= subprocess.PIPE)
        return self

    def partlist_multipart(self):
        my_bucket = "s3://" + self.bucket_name
        my_object = "s3://" + self.bucket_name + "/" + self.filename
        popenobj = subprocess.Popen(["s3cmd", "-c",self.s3cfg,"multipart",my_bucket], stdout=subprocess.PIPE, stderr= subprocess.PIPE)
        total_output = ""
        upload_id = ""
        while not popenobj.poll():
           data = popenobj.stdout.readline()
           stdoutdata = data.decode('utf-8')
           total_output += stdoutdata
           if stdoutdata:
             if( stdoutdata.find(my_object) >= 0):
               upload_id = stdoutdata[-37:-1]
               break
           else:
             break
        if upload_id:
          retry = 0
          while 1:
            popenlist = subprocess.Popen(["s3cmd", "-c",self.s3cfg,"listmp",my_object, upload_id],
                                       stdout=subprocess.PIPE, stderr= subprocess.PIPE)
            counter = 0
            while not popenlist.poll():
               listdata = popenlist.stdout.readline()
               stdoutdatalist = listdata.decode('utf-8')
               counter += 1
               if stdoutdatalist:
                 total_output = stdoutdatalist
               else:
                 break

            if(counter > 2 or retry > 8):
              if(counter > 2):
                print(total_output)
              else:
                raise RuntimeError("listing of parts failed")
              break
            retry += 1
            time.sleep(2)
        else:
          raise RuntimeError("listing of parts failed, upload id not found")
        return self

    def multipartupload_list_test(self, bucket_name, filename, filesize):
        self.filename = filename
        self.filesize = filesize
        self.bucket_name = bucket_name
        t = Timer(4, self.list_multipart)
        t.start()
        self.with_cli("s3cmd -c " + self.s3cfg + " put " + self.filename + " s3://" + self.bucket_name)
        return self

    def multipartupload_abort_test(self, bucket_name, filename, filesize):
        self.filename = filename
        self.filesize = filesize
        self.bucket_name = bucket_name
        t = Timer(4, self.abort_multipart)
        t.start()
        self.with_cli("s3cmd -c " + self.s3cfg + " put " + self.filename + " s3://" + self.bucket_name)
        return self

    def multipartupload_partlist_test(self, bucket_name, filename, filesize):
        self.filename = filename
        self.filesize = filesize
        self.bucket_name = bucket_name
        t = Timer(4, self.partlist_multipart)
        t.start()
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

    def multi_delete_test(self, bucket_name):
        self.bucket_name = bucket_name
        self.with_cli("s3cmd -c " + self.s3cfg + " del " + "s3://" + self.bucket_name + "/ --recursive --force")
        return self
