/*
 * COPYRIGHT 2017 SEAGATE LLC
 *
 * THIS DRAWING/DOCUMENT, ITS SPECIFICATIONS, AND THE DATA CONTAINED
 * HEREIN, ARE THE EXCLUSIVE PROPERTY OF SEAGATE TECHNOLOGY
 * LIMITED, ISSUED IN STRICT CONFIDENCE AND SHALL NOT, WITHOUT
 * THE PRIOR WRITTEN PERMISSION OF SEAGATE TECHNOLOGY LIMITED,
 * BE REPRODUCED, COPIED, OR DISCLOSED TO A THIRD PARTY, OR
 * USED FOR ANY PURPOSE WHATSOEVER, OR STORED IN A RETRIEVAL SYSTEM
 * EXCEPT AS ALLOWED BY THE TERMS OF SEAGATE LICENSES AND AGREEMENTS.
 *
 * YOU SHOULD HAVE RECEIVED A COPY OF SEAGATE'S LICENSE ALONG WITH
 * THIS RELEASE. IF NOT PLEASE CONTACT A SEAGATE REPRESENTATIVE
 * http://www.seagate.com/contact
 *
 * Original author: Sushant Mane <sushant.mane@seagate.com>
 * Original creation date: 03-Mar-2017
 */

package com.seagates3.util;

import com.seagates3.authserver.AuthServerConfig;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.lang.management.ManagementFactory;
import java.net.InetAddress;
import java.net.UnknownHostException;
import java.util.Date;

public class IEMUtil {

    public enum Level {INFO, ERROR, WARN}
    private static Logger logger = LoggerFactory.getLogger(BinaryUtil.class.getName());

    public static final String LDAP_EX = "048001001";
    public static final String UTF8_UNAVAILABLE = "048002001";
    public static final String HMACSHA256_UNAVAILABLE = "048002002";
    public static final String HMACSHA1_UNAVAILABLE = "048002003";
    public static final String SHA256_UNAVAILABLE = "048002004";
    public static final String UNPARSABLE_DATE = "048003001";
    public static final String CLASS_NOT_FOUND_EX = "048004001";
    public static final String NO_SUCH_METHOD_EX = "048004002";
    public static final String XML_SCHEMA_VALIDATION_ERROR = "048005001";


    public static void log(Level level, String eventCode,
                           String eventCodeString, String data) {
        String iem = generateIemString(eventCode, eventCodeString, data);
        switch (level) {
            case ERROR:
                logger.error(iem);
                break;
            case WARN:
                logger.warn(iem);
                break;
            case INFO:
                logger.info(iem);
        }
    }

    private static String generateIemString(String eventCode, String eventCodeString,
                                            String data) {
        String node = String.format("\"node\": \"%s\"", getHostName());
        String time = String.format("\"time\": \"%s\"", new Date().toString());
        String jvm = ManagementFactory.getRuntimeMXBean().getName();
        String pid = String.format("\"pid\": %s", jvm.substring(0, jvm.indexOf("@")));
        String jsonData = String.format("{ %s, %s, %s, %s }", time, node, pid, getLocation());
        if (data != null && !data.isEmpty()) {
            jsonData = String.format("{ %s, %s, %s, %s, %s }", time, node, pid, getLocation(), data);
        }
        String iem = String.format("IEC: %s: %s: %s", eventCode, eventCodeString, jsonData);

        return iem;
    }

    private static String getHostName() {
        String hostname = "";

        try {
            hostname = InetAddress.getLocalHost().getHostName();
        } catch (UnknownHostException e) {
            logger.info("Exception occurred while retrieving hostname.");
        }

        return hostname;
    }

    private static String getLocation() {
        StackTraceElement traceElement = Thread.currentThread().getStackTrace()[4];
        String location = String.format("\"file\": \"%s\", \"line\": %d",
                traceElement.getFileName(), traceElement.getLineNumber());

        return location;
    }
}
