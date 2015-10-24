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

package com.seagates3.validator;

public class ValidatorHelper {

    /*
     * To do
     * new user name have length between 1 and 64 character while user name
     * validation allows upto 128 char. Investigate this.
     */

    /*
     * This class contains validation rules for all the common input values.
     */

    /*
     * User name should be at least 1 character long and at max 128 char long.
     */
    public Boolean validName(String userName) {
        return !(userName.length() < 1 || userName.length() > 128);
    }

    /*
     * Account name should be at least 1 character long and at max 128 char long.
     */
    public Boolean validAccountName(String accountName) {
        return !(accountName.length() < 1 || accountName.length() > 128);
    }

    /*
     * Accepted values for status are 'Active' or 'Inactive'
     */
    public Boolean validStatus(String status) {
        return (status.equals("Active") || status.equals("Inactive"));
    }

    /*
     * Path should be at least 1 character long and at max 512 char long.
     */
    public Boolean validPath(String path) {
        return !(path.length() < 1 || path.length() > 512);
    }

    /*
     * Access Key Id should be at least 1 character long and at max 32 char long.
     */
    public Boolean validAccessKeyId(String accessKeyId) {
        return !(accessKeyId.length() < 1 || accessKeyId.length() > 32);
    }

    /*
     * Saml metadata document should be atleast 1000 character long and at max
     * 10000000 char long.
     */
    public Boolean validSAMLMetadata(String samlMetadataDocument) {
        return !(samlMetadataDocument.length() < 1000 ||
                samlMetadataDocument.length() > 10000000);
    }
}
