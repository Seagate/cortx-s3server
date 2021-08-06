/*
 * Copyright (c) 2021 Seagate Technology LLC and/or its Affiliates
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * For any questions about this software or licensing,
 * please email opensource@seagate.com or cortx-questions@seagate.com.
 *
 */
package com.seagates3.s3service;
import com.seagates3.authserver.AuthServerConfig;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import java.net.*;
import java.nio.charset.Charset;
import java.io.*;

public class AuditLogRestClient {

  private
   final static Logger LOGGER =
       LoggerFactory.getLogger(AuditLogRestClient.class.getName());

  public
   static void sendMessage(String auth_iam_audit_log) {
     // initialize socket and output streams
     Socket socket = null;
     DataOutputStream dataOutStream = null;
     String rsyslogHostname = AuthServerConfig.getRsyslogHostname();
     int rsyslogPortNUmber = AuthServerConfig.getRsyslogPort();
     try {
       // establish a connection
       LOGGER.debug("Creating socket");
       LOGGER.debug("rsyslog hostname : " + rsyslogHostname);
       LOGGER.debug("rsyslog port : " + String.valueOf(rsyslogPortNUmber));
       socket = new Socket(rsyslogHostname, rsyslogPortNUmber);
       LOGGER.debug("Socket Connected");

       LOGGER.debug("IAM Message to be sent for audit log : " +
                    auth_iam_audit_log);
       // sends message to the socket
       dataOutStream = new DataOutputStream(socket.getOutputStream());
       // Send Message
       dataOutStream.write(
           auth_iam_audit_log.getBytes(Charset.forName("UTF-8")));
       LOGGER.debug("IAM Message sent");
     }
     catch (Exception ex) {
       LOGGER.error("Failed to send IAM Audit log message : " + ex.toString());
     }
     finally {
       try {
         // close the connection
         dataOutStream.close();
         socket.close();
         LOGGER.debug("Socket Disconnected");
      }
      catch (IOException ex) {
        // No need to handle the exception
      }
    }
   }
}