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


USAGE="USAGE: $(basename "$0") [-I | -R | -S | -y <configure-yum-repo> | -p <path-to-prod-cfg> | -U | -D]

where:
-I	Install cortx-motr, cortx-motr-devel, cortx-hare, cortx-s3iamcli from rpms
	cortx-s3server will be built from local sources
-R	Remove cortx-motr, cortx-motr-devel, cortx-hare, cortx-s3iamcli, cortx-s3server, cortx-s3server-debuginfo packages
-S	Show status
-y <configure-yum-repo>	Configure yum repo to install cortx-motr and cortx-hare from cortx repos.
                       	'<sprint>' - i.e. 'Cortx-1.0.0-34-rc13' repos
-p <path-to-prod-cfg>  	Use path as s3server config
                       	@ - for s3server/s3config.release.yaml config
-U	Up - start cluster
-D	Down - stop cluster

Operations could be combined into single command, i.e.

$(basename "$0") -y Cortx-1.0.0-34-rc13 -RI -p @ -US

the command will update repos, remove pkgs, install pkgs, update s3server config,
run cluster and print statuses of all installed pkgs
"

usage() {
    echo "$USAGE" 1>&2;
    exit 1
}

set -e

SCRIPT_PATH=$(readlink -f "$0")
BASEDIR=$(dirname "$SCRIPT_PATH")

if rpm -q "salt"  > /dev/null 2>&1;
then
    # Release/Prod environment
    ldap_admin_pwd=$(salt-call pillar.get openldap:iam_admin:secret --output=newline_values_only)
    ldap_admin_pwd=$(salt-call lyveutil.decrypt openldap "${ldap_admin_pwd}" --output=newline_values_only)
else
    # Dev environment. Read ldap admin password from "/root/.s3_ldap_cred_cache.conf"
    source /root/.s3_ldap_cred_cache.conf
fi

USE_SUDO=
if [[ $EUID -ne 0 ]]; then
  command -v sudo || (echo "Script should be run as root or sudo required." && exit 1)
  USE_SUDO=sudo
fi

install_pkgs() {
    $USE_SUDO yum -t -y install cortx-motr
    $USE_SUDO yum -t -y install cortx-motr-devel
    $USE_SUDO yum -t -y install cortx-hare

    # option "-P" will build cortx-s3server rpm from the source tree
    # option "-i" will install cortx-s3server from the built rpm
    $USE_SUDO ${BASEDIR}/rpms/s3/buildrpm.sh -P ${BASEDIR} -i
    $USE_SUDO yum -t -y install cortx-s3iamcli
}

remove_pkgs() {
    $USE_SUDO yum -t -y remove cortx-s3iamcli cortx-s3server-debuginfo cortx-s3server cortx-hare cortx-motr-devel cortx-motr 
}

sysctl_stat() {
    $USE_SUDO systemctl status $1 &> /dev/null
    local st=$?
    case $st in
        0) echo "running" ;;
        3) echo "stopped" ;;
        *) echo "error" ;;
    esac
}

haproxy_ka_status() {
    $USE_SUDO grep -e "^\s*#\s*option httpchk HEAD" /etc/haproxy/haproxy.cfg &> /dev/null && echo "disabled"
    $USE_SUDO grep -e "^\s*option httpchk HEAD" /etc/haproxy/haproxy.cfg &> /dev/null && echo "enabled"
    $USE_SUDO grep -e "option httpchk HEAD" /etc/haproxy/haproxy.cfg &> /dev/null || echo "option not found"
}

status_srv() {
    case "$1" in
        cortx-s3server) echo -e "\t\t PIDs:> $(pgrep $1 | tr '\n' ' ')"
                  echo -e "\t\t s3authserver:> $(sysctl_stat s3authserver)"
                  echo -e "\t\t $($USE_SUDO grep -o -e "S3_LOG_MODE:\s*\S*" /opt/seagate/cortx/s3/conf/s3config.yaml)"
                  echo -e "\t\t $($USE_SUDO grep -o -e "S3_LOG_ENABLE_BUFFERING:\s*\S*" /opt/seagate/cortx/s3/conf/s3config.yaml)"
                  ;;
        cortx-haproxy) echo -e "\t\t $(sysctl_stat $1)"
                 echo -e "\t\t keepalive $(haproxy_ka_status)"
                 ;;
        cortx-hare) echo -e "\t\t hared:> $(sysctl_stat hared)"
               ;;
        openldap) echo -e "\t\t $(sysctl_stat slapd)"
                  ;;
        cortx-motr) $USE_SUDO hctl cortx-motr status
              ;;
        *)
              ;;
    esac
}

status_pkgs() {
    for test_pkg in cortx-s3server-debuginfo cortx-s3server cortx-hare cortx-motr-devel haproxy openldap cortx-motr
    do
        set +e
        $USE_SUDO yum list installed $test_pkg &> /dev/null
        local inst_stat=$?
        echo "Package $test_pkg"
        [ $inst_stat == 0 ] && echo -e "\t\t installed" || echo -e "\t\t not found"
        [ $inst_stat == 0 ] && status_srv $test_pkg
        set -e
    done
}

yum_repo_conf() {
    case "$1" in
        cortx) $USE_SUDO rm -f /etc/yum.repos.d/releases_sprint.repo || true
               for f in /etc/yum.repos.d/*cortx*repo; do $USE_SUDO sed -i 's/priority.*/priority = 1/' $f; done
               for f in /etc/yum.repos.d/*s3server*repo; do $USE_SUDO sed -i 's/priority.*/priority = 1/' $f; done
               ;;
        ees*-sprint*) echo "Use sprint $1 builds"
                      $USE_SUDO echo "[sprints_s3server]
baseurl = http://cortx-storage.colo.seagate.com/releases/cortx/${1}/
gpgcheck = 0
name = Yum repo for cortx-s3server sprints build
priority = 1

[sprints_hare]
baseurl = http://cortx-storage.colo.seagate.com/releases/cortx/${1}/
gpgcheck = 0
name = Yum repo for cortx-hare sprints build
priority = 1

[sprints_motr]
baseurl = http://cortx-storage.colo.seagate.com/releases/cortx/${1}/
gpgcheck = 0
name = Yum repo for motr sprints build
priority = 1
" > /etc/yum.repos.d/releases_sprint.repo
                      for f in /etc/yum.repos.d/*cortx*repo; do $USE_SUDO sed -i 's/priority.*/priority = 2/' $f; done
                      for f in /etc/yum.repos.d/*s3server*repo; do $USE_SUDO sed -i 's/priority.*/priority = 2/' $f; done
                      ;;
        *) echo "Invalid sprint name provided"
           ;;
    esac
    $USE_SUDO yum clean all
    $USE_SUDO  rm -rf /var/cache/yum
}

up_cluster() {
    $USE_SUDO ./scripts/enc_ldap_passwd_in_cfg.sh -l "$ldap_admin_pwd" -p /opt/seagate/cortx/auth/resources/authserver.properties
    $USE_SUDO systemctl start haproxy
    $USE_SUDO systemctl start slapd
    $USE_SUDO systemctl start s3authserver
    $USE_SUDO rm -fR /var/motr/*
    $USE_SUDO m0setup -P 1 -N 1 -K 0 -vH
    $USE_SUDO sed -i 's/- name: m0t1fs/- name: s3server/' /etc/hare/hare_facts.yaml
    $USE_SUDO awk 'BEGIN {in_section=0} {if ($0 ~ /^- name:/) {in_section=0; if ($0 ~ /^- name: "s3server"/) {in_section=1;} } {if (in_section==1) {gsub("multiplicity: 4", "multiplicity: 2");} print;} }' /etc/hare/motr_role_mappings > /tmp/motr_role_mappings
    $USE_SUDO cp /tmp/motr_role_mappings /etc/hare/motr_role_mappings
    $USE_SUDO rm /tmp/motr_role_mappings
    $USE_SUDO systemctl start hare-cleanup
    $USE_SUDO systemctl start hared
    $USE_SUDO hctl cortx-motr bootstrap
    sleep 2
    $USE_SUDO hctl cortx-motr status
}

down_cluster() {
    $USE_SUDO hctl cortx-motr stop
    $USE_SUDO systemctl stop hared
    $USE_SUDO systemctl stop hare-cleanup
    $USE_SUDO systemctl stop s3authserver
    $USE_SUDO systemctl stop slapd
    $USE_SUDO systemctl stop haproxy
}

while getopts ":IRSy:p:UD" o; do
    case "${o}" in
        I) echo "Installing..."
           install_pkgs
           ;;
        R) echo "Removing..."
           remove_pkgs
           ;;
        S) echo "Status..."
           status_pkgs
           ;;
        y) echo "Confirure yum repos to use <${OPTARG}>..."
           yum_repo_conf ${OPTARG}
           ;;
        p) copy_path=${OPTARG}
           if [ "$copy_path" == "@" ]; then
               copy_path=${BASEDIR}/s3config.release.yaml
           fi
           echo "Copy config ${copy_path} to /opt/seagate/cortx/s3/conf/s3config.yaml"
           $USE_SUDO mkdir -p /opt/seagate/cortx/s3/conf/
           [ -f /opt/seagate/cortx/s3/conf/s3config.yaml ] && $USE_SUDO cp -f /opt/seagate/cortx/s3/conf/s3config.yaml /opt/seagate/cortx/s3/conf/s3config.yaml_bak
           $USE_SUDO cp "${copy_path}" /opt/seagate/cortx/s3/conf/s3config.yaml
           ;;
        U) echo "Up"
           up_cluster
           ;;
        D) echo "Down"
           down_cluster
           ;;
        *) usage
           ;;
    esac
done
shift $((OPTIND-1))
