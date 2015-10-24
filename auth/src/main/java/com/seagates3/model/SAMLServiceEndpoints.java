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
 * Original creation date: 14-oct-2015
 */

package com.seagates3.model;

public class SAMLServiceEndpoints {

    public enum Binding {
        HTTP_REDIRECT("HTTP-REDIRECT"),
        HTTP_POST("HTTP-POST");

        private final String binding;

        private Binding(final String binding) {
            this.binding = binding;
        }

        @Override
        public String toString() {
            return binding;
        }
    }

    public enum ServiceType {
        SINGLE_LOGOUT_SERVICE("SingleLogoutService"),
        SINGLE_SIGNON_SERVICE("SingleSignOnService");

        private final String serviceType;

        private ServiceType(final String serviceType) {
            this.serviceType = serviceType;
        }

        @Override
        public String toString() {
            return serviceType;
        }
    }

    private String location;
    private Binding binding;
    private ServiceType serviceType;

    public String getBinding() {
        return binding.toString();
    }

    public String getLocation() {
        return location;
    }

    public String getService() {
        return serviceType.toString();
    }

    public void setBinding(Binding binding) {
        this.binding = binding;
    }

    public void setLocation(String location) {
        /*
         * Location should be a valid url
         */

        this.location = location;
    }

    public void setServiceType(ServiceType serviceType) {
        this.serviceType = serviceType;
    }
}
