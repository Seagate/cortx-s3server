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
 * Original creation date: 14-Oct-2015
 */

package com.seagates3.model;

import java.security.cert.Certificate;

public class KeyDescriptor {

    public enum KeyUse {
        ENCRYPTION("encryption"),
        SIGNING("signing");

        private final String use;

        private KeyUse(final String text) {
            this.use = text;
        }

        @Override
        public String toString() {
            return use;
        }
    }


    private Certificate[] certificates;
    private KeyUse use;

    public KeyUse getUse() {
        return use;
    }

    public Certificate[] getCertificate() {
        return certificates;
    }

    public void setUse(KeyUse use) {
        this.use = use;
    }

    public void setCertificate(Certificate[] certificates) {
        /*
         * check if the certificate is valid
         */

        this.certificates = certificates;
    }

}
