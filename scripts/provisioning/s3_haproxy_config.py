#!/usr/bin/env python3
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

import os
import sys
import socket
from s3confstore.cortx_s3_confstore import S3CortxConfStore
import logging
import configparser

class S3HaproxyConfig:
  """HAProxy configration for S3."""
  local_confstore = None
  provisioner_confstore = None
  machine_id = None

  def __init__(self, confstore: str):
    """Constructor."""

    s3deployment_logger_name = "s3-deployment-logger-" + "[" + str(socket.gethostname()) + "]"
    self.logger = logging.getLogger(s3deployment_logger_name)
    if self.logger.hasHandlers():
      self.logger.info("Logger has valid handler")
    else:
      self.logger.setLevel(logging.DEBUG)
      # create console handler with a higher log level
      chandler = logging.StreamHandler(sys.stdout)
      chandler.setLevel(logging.DEBUG)
      s3deployment_log_format = "%(asctime)s - %(name)s - %(levelname)s - %(message)s"
      formatter = logging.Formatter(s3deployment_log_format)
      # create formatter and add it to the handlers
      chandler.setFormatter(formatter)
      # add the handlers to the logger
      self.logger.addHandler(chandler)

    # Read machine-id of current node
    with open('/etc/machine-id', 'r') as mcid_file:
      self.machine_id = mcid_file.read().strip()

    if not confstore.strip():
      self.logger.error(f'config url:[{confstore}] must be a valid url path')
      raise Exception('empty config URL path')

    self.provisioner_confstore = S3CortxConfStore(confstore, 'haproxy_config_index')

  def get_publicip(self):
    assert self.provisioner_confstore != None
    assert self.local_confstore != None

    return self.provisioner_confstore.get_config(
      self.local_confstore.get_config(
        'CONFIG>CONFSTORE_PUBLIC_FQDN_KEY').replace("machine-id", self.machine_id))

  def get_privateip(self):
    assert self.provisioner_confstore != None
    assert self.local_confstore != None

    return self.provisioner_confstore.get_config(
      self.local_confstore.get_config(
        'CONFIG>CONFSTORE_PRIVATE_FQDN_KEY').replace("machine-id", self.machine_id))

  def get_s3instances(self):
    assert self.provisioner_confstore != None
    assert self.local_confstore != None

    return int(self.provisioner_confstore.get_config(
      self.local_confstore.get_config(
        'CONFIG>CONFSTORE_S3INSTANCES_KEY').replace("machine-id", self.machine_id)))

  def process(self):
    """Main Processing function."""
    # provisioner is creating override.conf file for haproxy in which
    # restart should be 'no' as it is managed by HA
    self.logger.info('Set haproxy restart to NO started')
    self.set_haproxy_restart_to_no()
    self.logger.info('Set haproxy restart to NO completed')

    self.local_confstore = S3CortxConfStore(
      "yaml:///opt/seagate/cortx/s3/mini-prov/s3_prov_config.yaml",
      'localstore')

    #Get necessary info from confstore
    localhost = '127.0.0.1'
    pvt_ip = self.get_privateip()
    pub_ip = self.get_publicip()
    numS3Instances = self.get_s3instances()

    if numS3Instances <= 0:
        numS3Instances = 1

    #Initialize header and footer text
    header_text = "#-------S3 Haproxy configuration start---------------------------------"
    footer_text = "#-------S3 Haproxy configuration end-----------------------------------"

    #Remove all existing content from header to footer
    is_found = False
    header_found = False
    original_file = "/etc/haproxy/haproxy.cfg"
    dummy_file = original_file + '.bak'
    # Open original file in read only mode and dummy file in write mode
    with open(original_file, 'r') as read_obj, open(dummy_file, 'w') as write_obj:
        # Line by line copy data from original file to dummy file
        for line in read_obj:
            line_to_match = line
            if line[-1] == '\n':
                line_to_match = line[:-1]
            # if current line matches with the given line then skip that line
            if line_to_match != header_text:
                if header_found == False:
                    write_obj.write(line)
                elif line_to_match == footer_text:
                     header_found = False
            else:
                is_found = True
                header_found = True
                break
    read_obj.close()
    write_obj.close()
    # If any line is skipped then rename dummy file as original file
    if is_found:
        os.remove(original_file)
        os.rename(dummy_file, original_file)
    else:
        os.remove(dummy_file)

    #Set the string literals to be added to config file
    frontend_s3main_text = '''
#----------------------------------------------------------------------
# FrontEnd S3 Configuration
#----------------------------------------------------------------------
frontend s3-main
    # s3 server port
'''
    backend_s3main_text = '''

    option forwardfor
    default_backend s3-main

    # s3 auth server port
    bind 0.0.0.0:9080
    bind 0.0.0.0:9443 ssl crt /etc/ssl/stx/stx.pem

    acl s3authbackendacl dst_port 9443
    acl s3authbackendacl dst_port 9080
    use_backend s3-auth if s3authbackendacl

#----------------------------------------------------------------------
# BackEnd roundrobin as balance algorithm
#----------------------------------------------------------------------
backend s3-main
    balance static-rr                                     #Balance algorithm
    http-response set-header Server SeagateS3
    # Check the S3 server application is up and healthy - 200 status code
    option httpchk HEAD / HTTP/1.1\\r\\nHost:\ localhost

    # option log-health-checks
    default-server inter 2s fastinter 100 rise 1 fall 5 on-error fastinter

    # For ssl communication between haproxy and s3server
    # Replace below line
'''

    backend_s3auth_text = '''
#----------------------------------------------------------------------
# BackEnd roundrobin as balance algorith for s3 auth server
#----------------------------------------------------------------------
backend s3-auth
    balance static-rr                                     #Balance algorithm

    # Check the S3 Auth server application is up and healthy - 200 status code
    option httpchk HEAD /auth/health HTTP/1.1\\r\\nHost:\ localhost

    # option log-health-checks
    default-server inter 2s fastinter 100 rise 1 fall 5 on-error fastinter

'''

    new_line_text = '''
'''

    #Initialize port numbers
    s3inport = 28071
    s3auport = 28050

    #Add complete information to haproxy.cfg file
    cfg_file = '/etc/haproxy/haproxy.cfg'
    target = open(cfg_file, "a+")

    target.write(header_text)
    target.write(frontend_s3main_text)
    target.write(
        "    bind %s:80 ##### localhost 80 required for Auth - S3 connection\n"
        "    bind %s:443 ssl crt /etc/ssl/stx/stx.pem ### localhost required for CSM/UDX\n"
        "    bind %s:80\n"
        "    bind %s:443 ssl crt /etc/ssl/stx/stx.pem\n"
        "    bind %s:80\n"
        "    bind %s:443 ssl crt /etc/ssl/stx/stx.pem\n"
        % (localhost, localhost, pvt_ip, pvt_ip, pub_ip, pub_ip))
    target.write(backend_s3main_text)
    for i in range(0, numS3Instances):
        target.write(
        "    server s3-instance-%s %s:%s check maxconn 110        # s3 instance %s\n"
        % (i+1, pvt_ip, s3inport+i, i+1))
    target.write(backend_s3auth_text)
    for i in range(0, 1):
        target.write(
        "    server s3authserver-instance%s %s:%s #check ssl verify required ca-file /etc/ssl/stx-s3/s3auth/s3authserver.crt   # s3 auth server instance %s\n"
        % (i+1, pvt_ip, s3auport+i, i+1))
    target.write(new_line_text)
    target.write(footer_text)

    target.close()

    #Check for destination dirs and create if needed
    if not os.path.exists('/etc/haproxy/errors/'):
        os.makedirs('/etc/haproxy/errors/')
    if not os.path.exists('/etc/logrotate.d/haproxy'):
        os.makedirs('/etc/logrotate.d/haproxy')
    if not os.path.exists('/etc/rsyslog.d/haproxy.conf'):
        os.makedirs('/etc/rsyslog.d/haproxy.conf')
    if not os.path.exists('/etc/cron.hourly/logrotate'):
        os.makedirs('/etc/cron.hourly/logrotate')
    if not os.path.exists('/etc/cron.daily/logrotate'):
        os.makedirs('/etc/cron.daily/logrotate')

    #Run config commands
    os.system("cp /opt/seagate/cortx/s3/install/haproxy/503.http /etc/haproxy/errors/")
    os.system("cp /opt/seagate/cortx/s3/install/haproxy/logrotate/haproxy /etc/logrotate.d/haproxy")
    os.system("cp /opt/seagate/cortx/s3/install/haproxy/rsyslog.d/haproxy.conf /etc/rsyslog.d/haproxy.conf")
    os.system("rm -rf /etc/cron.daily/logrotate")
    os.system("cp /opt/seagate/cortx/s3/install/haproxy/logrotate/logrotate /etc/cron.hourly/logrotate")
    os.system("systemctl restart rsyslog")

  def set_haproxy_restart_to_no(self):
    """Set restart to 'no' in /etc/systemd/system/haproxy.service.d/override.conf."""
    overridefilepath = "/etc/systemd/system/haproxy.service.d/override.conf"
    if os.path.isfile(overridefilepath):
      self.logger.info(f"file {overridefilepath} exist")
      config = configparser.ConfigParser()
      config.read(overridefilepath)
      config.set("Service", "Restart", "no")
      with open(overridefilepath, 'w') as configfile:
          config.write(configfile)
