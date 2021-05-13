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
#

# function to validate and configure lnet
validte_configure_lnet() {
  lctl list_nids | grep "@tcp" &> /dev/null
  if [ $? -eq 0 ]; then
    echo "Lnet is up and running"
  else
    echo "Lnet is down."
    echo "configuring lnet"
    echo "options lnet networks=tcp(eth0) config_on_load=1" > '/etc/modprobe.d/lnet.conf'
    service lnet restart
    # it has been observed that the lnet does not get restart in first go
    # so for safer side restarting again and then verify
    service lnet restart
    lctl list_nids | grep "@tcp" &> /dev/null
    if [ $? -eq 0 ]; then
      echo "Lnet is up and running"
    else
      echo "Lnet is down after configuring too"
    fi
  fi
}

###############################
### Main script starts here ###
###############################

# validate and configure lnet
validte_configure_lnet