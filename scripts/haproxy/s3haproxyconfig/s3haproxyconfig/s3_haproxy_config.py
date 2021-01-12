#!/usr/bin/python3

import os
import sys
import fileinput
import socket
from cortx.utils.conf_store import Conf

class S3HaproxyConfig:

  def run(self):
    #Check cluster status
    #hctl_status=os.popen("hctl status").read()
    #hctl_not_running='Cluster is not running'
    #hctl_not_installed='command not found'
    #if (
    #    str(hctl_status.rstrip("\n")) == str(hctl_not_running) or 
    #    str(hctl_status.rstrip("\n")) == str(hctl_not_installed)
    #   ):
    #    print("Cluster error")
    #    exit(-1)

    #Get private IP from confstore
    index = 'default_index'
    Conf.load(index, 'json:///tmp/cortx_s3_confstore.json')
    pvt_ip = Conf.get(index, 'cluster>server>network>data>private_ip')

    #Remove content not valid for use
    header_text = "#HAProxy Monitoring Config"
    footer_text = "#HAProxy Monitoring Config end"
    is_skipped = False
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
                write_obj.write(line)
            else:
                is_skipped = True
                break
    read_obj.close()
    write_obj.close()
    # If any line is skipped then rename dummy file as original file
    if is_skipped:
        os.remove(original_file)
        os.rename(dummy_file, original_file)
    else:
        os.remove(dummy_file)

    #Add backend information to haproxy.cfg file
    cfg_file = '/etc/haproxy/haproxy.cfg'
    target = open(cfg_file, "a+")
    target.write('''# BackEnd roundrobin as balance algorithm
#---------------------------------------------------------------------
backend app-main
    balance static-rr                                     #Balance algorithm
    http-response set-header Server SeagateS3
    # Check the S3 server application is up and healthy - 200 status code
    option httpchk HEAD / HTTP/1.1\\r\\nHost:\ localhost

    # option log-health-checks
    default-server inter 2s fastinter 100 rise 1 fall 5 on-error fastinter

    # For ssl communication between haproxy and s3server
    # Replace below line
    server s3-instance-1 ''')
    target.write(pvt_ip)
    target.write(''':28081 check maxconn 110        # s3 instance 1


#----------------------------------------------------------------------
# BackEnd roundrobin as balance algorith for s3 auth server
#----------------------------------------------------------------------
backend s3-auth
    balance static-rr                                     #Balance algorithm

    # Check the S3 Auth server application is up and healthy - 200 status code
    option httpchk HEAD /auth/health HTTP/1.1\\r\\nHost:\ localhost

    # option log-health-checks
    default-server inter 2s fastinter 100 rise 1 fall 5 on-error fastinter

    server s3authserver-instance1 ''')
    target.write(pvt_ip)
    target.write(''':9085 #check ssl verify required ca-file /etc/ssl/stx-s3/s3auth/s3authserver.crt   # s3 auth server instance 1
#----------------------------------------------------------------------
''')

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
    os.system("mkdir /etc/haproxy/errors/")
    os.system("cp /opt/seagate/cortx/s3/install/haproxy/503.http /etc/haproxy/errors/")
    os.system("cp /opt/seagate/cortx/s3/install/haproxy/logrotate/haproxy /etc/logrotate.d/haproxy")
    os.system("cp /opt/seagate/cortx/s3/install/haproxy/rsyslog.d/haproxy.conf /etc/rsyslog.d/haproxy.conf")
    os.system("rm -rf /etc/cron.daily/logrotate")
    os.system("cp /opt/seagate/cortx/s3/install/haproxy/logrotate/logrotate /etc/cron.hourly/logrotate")
    os.system("systemctl restart rsyslog")
    os.system("systemctl restart haproxy")
    os.system("systemctl status haproxy")

