#!/usr/bin/python3.6
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
import fileinput
import argparse
from cortx.utils.conf_store import Conf
from s3confstore.cortx_s3_confstore import S3CortxConfStore

class S3HaproxyConfig:

  @staticmethod
  def run():
    parser = argparse.ArgumentParser(description='S3 haproxy configuration')
    parser.add_argument("--path", required=True, help='cortx-py-utils:confstore back-end file URL.', type=str)

    args = parser.parse_args()

    if args.path and args.path.strip():
      s3conf_store = S3CortxConfStore(args.path)
    else:
      sys.exit("--path option value:[{}] is not valid.".format(args.path))

    #Read machine-id of current node
    mcid_file = open('/etc/machine-id', 'r')
    machine_id = mcid_file.read().strip()
    mcid_file.close()

    #Get necessary info from s3conf_store
    localhost = '127.0.0.1'
    pvt_ip = s3conf_store.get_privateip(machine_id)
    pub_ip = s3conf_store.get_publicip(machine_id)
    numS3Instances = int(s3conf_store.get_s3instance_count(machine_id))
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
    #bind 0.0.0.0:9443 ssl crt /etc/ssl/stx/stx.pem

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
    s3inport = 28081
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
    os.system("systemctl restart haproxy")

