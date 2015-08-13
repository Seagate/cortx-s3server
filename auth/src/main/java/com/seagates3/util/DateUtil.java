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

public class DateUtil {

    public static String toLdapDate(Date date) {
        SimpleDateFormat sdf = new SimpleDateFormat("yyyyMMddHHmmss");
        return (sdf.format(date) + "Z");
    }

    public static String toServerResponseFormat(Date date) {
        SimpleDateFormat serverResponseFormat = new
            SimpleDateFormat("yyyy-MM-dd'T'HH:mm:ss.SSSZ");

        return serverResponseFormat.format(date);
    }

    public static String toServerResponseFormat(String ldapDate) {
        Date date = toDate(ldapDate);
        SimpleDateFormat serverResponseFormat = new
            SimpleDateFormat("yyyy-MM-dd'T'HH:mm:ss.SSSZ");

        return serverResponseFormat.format(date);
    }

    public static Date toDate(String ldapdate) {
        SimpleDateFormat sdf = new SimpleDateFormat("yyyyMMddHHmmss");

        try {
            return sdf.parse(ldapdate);
        } catch (ParseException e) {
        }

        return null;
    }
}
