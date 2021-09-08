#!/bin/sh
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

# This is a temporary script which can be used to perform additional
# openldap related cleanups along with 's3setup --cleanup'.
# Since s3setup --cleanup is not idempotent yet, this cleanup script
# helps to rerun init and config after the cleanup.
# This script can be used along with another temp automate setup script
# scripts/provisioning/s3setup/test_s3_setup.sh

die_with_error () {
        echo "$1"
        exit -1
}

# Attempt ldap clean up since ansible openldap setup is not idempotent
systemctl stop slapd 2>/dev/null
# remove old openldap pkg if installed
yum remove -y openldap-servers openldap-clients || /bin/true
yum remove -y symas-openldap symas-openldap-servers symas-openldap-clients

rm -f /etc/openldap/slapd.d/cn\=config/cn\=schema/cn\=\{2\}s3user.ldif
rm -rf /var/lib/ldap/*
rm -f /etc/sysconfig/slapd* 2>/dev/null || /bin/true
rm -f /etc/openldap/slapd* 2>/dev/null || /bin/true
rm -rf /etc/openldap/slapd.d/*

yum install -y symas-openldap symas-openldap-servers symas-openldap-clients
systemctl start slapd 2>/dev/null || die_with_error "slapd could not be started!"
