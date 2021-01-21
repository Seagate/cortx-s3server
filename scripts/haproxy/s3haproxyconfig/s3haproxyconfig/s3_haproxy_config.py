#!/usr/bin/python3

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
      mcid_file = open('/etc/machine-id', 'r')
      machine_id = mcid_file.read().strip()
      mcid_file.close()
      pvt_ip = s3conf_store.get_privateip(machine_id)
    else:
      sys.exit("--path option value:[{}] is not valid.".format(args.path))

    #Remove content not valid for use
    header_text = "#-------S3 Haproxy configuration start---------------------------------"
    footer_text = "#-------S3 Haproxy configuration end-----------------------------------"
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

    #Add backend information to haproxy.cfg file
    cfg_file = '/etc/haproxy/haproxy.cfg'
    target = open(cfg_file, "a+")

    n = 1
    env = s3conf_store.get_config("cluster>env_type")

    if env == 'VM':
        n = 4
    elif env == 'HW':
        n = 11

    s3inport = 28081
    s3auport = 28050

    str1 = '''
#----------------------------------------------------------------------
# FrontEnd S3 Configuration
#----------------------------------------------------------------------
frontend s3-main
    # s3 server port
    bind 0.0.0.0:80
    #bind 0.0.0.0:443 ssl crt /etc/ssl/stx/stx.pem

    option forwardfor
    default_backend app-main

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

    str2 = '''
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

    str3 = '''
'''

    target.write(header_text)
    target.write(str1)
    for i in range(0, n):
        target.write(
        "    server s3-instance-%s %s:%s check maxconn 110        # s3 instance %s\n"
        % (i+1, pvt_ip, s3inport+i, i+1))
    target.write(str2)
    for i in range(0, n):
        target.write(
        "    server s3authserver-instance%s %s:%s #check ssl verify required ca-file /etc/ssl/stx-s3/s3auth/s3authserver.crt   # s3 auth server instance %s\n"
        % (i+1, pvt_ip, s3auport+i, i+1))
    target.write(str3)
    target.write(footer_text)
    target.close()

    #########################################
    # begin s3_instance_ip_update
    # Below snippet will convert the server
    # instance IP from 0.0.0.0 to the
    # IP addresses (eth0) of the current node

    #lineToSearch = 'server s3-instance-1'
    #textToSearch = '0.0.0.0'
    #hostname = socket.gethostname()
    #textToReplace = socket.gethostbyname(hostname)

    # Actual file
    #fileToSearch  = '/etc/haproxy/haproxy.cfg'

    #tempFile = open( fileToSearch, 'r+' )

    #for line in fileinput.input( fileToSearch ):
    #    if lineToSearch in line :
    #        if textToSearch in line :
    #            tempFile.write( line.replace( textToSearch, textToReplace ) )
    #tempFile.close()
    #  end s3_instance_ip_update
    ########################################

    ########################################
    # begin comment_monitoring_config
    # Below snippet will comment out the
    # HAProxy Monitoring Config section
    # of /etc/haproxy/haproxy.cfg

    #lineToSearch = 'HAProxy Monitoring Config'
    #textToReplace = '#'

    #fileToSearch  = 'sample'

    #for line in fileinput.input( fileToSearch ):
    #    if lineToSearch in line :
    #        tempFile.write( line.replace( textToSearch, textToReplace ) )
    #tempFile.close()
    # begin comment_monitoring_config
    ########################################

    #Run config commands
    if not os.path.exists('/etc/haproxy/errors/'):
        os.makedirs('/etc/haproxy/errors/')
    os.system("cp /opt/seagate/cortx/s3/install/haproxy/503.http /etc/haproxy/errors/")
    os.system("cp /opt/seagate/cortx/s3/install/haproxy/logrotate/haproxy /etc/logrotate.d/haproxy")
    os.system("cp /opt/seagate/cortx/s3/install/haproxy/rsyslog.d/haproxy.conf /etc/rsyslog.d/haproxy.conf")
    os.system("rm -rf /etc/cron.daily/logrotate")
    os.system("cp /opt/seagate/cortx/s3/install/haproxy/logrotate/logrotate /etc/cron.hourly/logrotate")
    os.system("systemctl restart rsyslog")
    os.system("systemctl restart haproxy")

