/*
 * COPYRIGHT 2015 SEAGATE LLC
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
 * Original author:  Arjun Hariharan <arjun.hariharan@seagate.com>
 * Original creation date: 17-Sep-2014
 */
package com.seagates3.util;

import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.TimeZone;
import java.util.logging.Level;
import java.util.logging.Logger;
import org.joda.time.DateTime;
import org.joda.time.format.DateTimeFormat;
import org.joda.time.format.DateTimeFormatter;

/*
 * TODO
 * replace java.util.date with org.joda.time.DateTime.
 */
public class DateUtil {

    private static final String LDAP_DATE_FORMAT = "yyyyMMddHHmmss";
    private static final String SERVER_RESPONSE_DATE_FORMAT
            = "yyyy-MM-dd'T'HH:mm:ss.SSSZ";

    public static String toLdapDate(Date date) {
        SimpleDateFormat sdf = getSimpleDateFormat(LDAP_DATE_FORMAT);
        return (sdf.format(date) + "Z");
    }

    public static String toLdapDate(String date) {
        return toLdapDate(toDate(date));
    }

    public static String toServerResponseFormat(Date date) {
        SimpleDateFormat serverResponseFormat
                = getSimpleDateFormat(SERVER_RESPONSE_DATE_FORMAT);

        return serverResponseFormat.format(date);
    }

    public static String toServerResponseFormat(DateTime date) {
        DateTimeFormatter fmt = DateTimeFormat.forPattern(
                SERVER_RESPONSE_DATE_FORMAT);
        return fmt.print(date);
    }

    public static String toServerResponseFormat(String ldapDate) {
        return toServerResponseFormat(toDate(ldapDate));
    }

    /*
     * Return the current time in UTC.
     */
    public static long getCurrentTime() {
        SimpleDateFormat dateFormatGmt = getSimpleDateFormat("yyyy-MMM-dd HH:mm:ss");
        dateFormatGmt.setTimeZone(TimeZone.getTimeZone("UTC"));

        try {
            return dateFormatGmt.parse(dateFormatGmt.format(new Date())).getTime();
        } catch (ParseException ex) {
            Logger.getLogger(DateUtil.class.getName()).log(Level.SEVERE, null, ex);
        }

        return 0;
    }

    public static Date toDate(String date) {
        SimpleDateFormat sdf = getSimpleDateFormat(LDAP_DATE_FORMAT);

        try {
            return sdf.parse(date);
        } catch (ParseException e) {
        }

        sdf = getSimpleDateFormat(SERVER_RESPONSE_DATE_FORMAT);
        try {
            return sdf.parse(date);
        } catch (ParseException e) {
        }

        return null;
    }

    private static SimpleDateFormat getSimpleDateFormat(String pattern) {
        SimpleDateFormat sdf = new SimpleDateFormat(pattern);
        sdf.setTimeZone(getDefatulTimeZone());

        return sdf;
    }

    /**
     * Return the default time zone (UTC).
     *
     * @return UTC Time zone.
     */
    private static TimeZone getDefatulTimeZone() {
        return TimeZone.getTimeZone("UTC");
    }
}
