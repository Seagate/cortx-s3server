#!/bin/bash -e
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
# shell automation script for https://github.com/Seagate/cortx-s3server/wiki/S3server-provisioning-on-single-node-VM-cluster:-Manual

die_with_error () {
	echo "$1 Exitting.." >&2
	exit -1
}

step_2 () {
	# Try to install cortx-py-utils and pip3 cryptography pkgs
	yum install cortx-s3server cortx-py-utils --nogpgcheck -y || die_with_error "cortx-py-utils install failed"
	pip3 install cryptography==2.8 || die_with_error "pip3 cryptography install failed"
}

step_3 () {
	# Update /etc/hosts
	loopback_ip="127.0.0.1"
	file="/etc/hosts"
	s3_host_entries="seagatebucket.s3.seagate.com seagate-bucket.s3.seagate.com
			 seagatebucket123.s3.seagate.com seagate.bucket.s3.seagate.com
			 s3-us-west-2.seagate.com seagatebucket.s3-us-west-2.seagate.com
			 iam.seagate.com sts.seagate.com s3.seagate.com"
	defaulthost=false
	s3_host_default="$(hostname -I | cut -d' ' -f1)"
	s3_host_ip=""

	if [ ! -z "$1" ];then
		echo "Using $1 as ipv4 address for \"$HOSTNAME\" "
		s3_host_ip=$1
		echo "$s3_host_ip"
		defaulthost=true
	fi

	if [[ $defaulthost == false ]]
	then
	  read -p "Enter ipv4 address for host \"$HOSTNAME\" (default is $s3_host_default): " s3_host_ip
	  [ "$s3_host_ip" == "" ] && s3_host_ip=$s3_host_default
	fi

	#Check whether given host is reachable.
	if ! ping -c1 "$s3_host_ip" &> /dev/null
	then
	  echo "Host $s3_host_ip is unreachable"
	  exit 1
	else
	  echo "Host $s3_host_ip is reachable"
	fi


	#Check whether multiple localhost entries are present or hostname is already configured.
	loopback_ip_count=$(grep "^$loopback_ip" /etc/hosts|wc -l)
	if [ "$loopback_ip_count" -ne 1 ]
	then
	  echo "\"$loopback_ip\" is used $loopback_ip_count times"
	  exit 1
	fi

	s3_host=$(grep "$HOSTNAME" /etc/hosts|wc -l)
	if [ "$s3_host" -ne 0 ]
	then
	  echo "\"$HOSTNAME\" is already configured, cannot be added again to /etc/hosts"
	fi

	#Backup original /etc/hosts file
	cp -f /etc/hosts /etc/hosts.backup.$(date +"%Y%m%d_%H%M%S")

	#Checking for duplication & adding s3 host entry
	for host_entry in $s3_host_entries
	do
	  # search_entry is for escaping '.'
	  search_entry="$(echo "$host_entry" | sed 's/\./\\./g')"
	  found=$(egrep "(^|\s+)$search_entry(\s+|$)" $file |wc -l)
	  if [ "$found" -eq 0 ]
	  then
	    sed -i "/^$loopback_ip/s/$/ $search_entry/" $file
	  else
	    echo "\"$host_entry\" is already configured, cannot be added again to /etc/hosts"
	  fi
	done

	#To add s3 hostname entry to /etc/hosts
	if [ "$s3_host_ip" = "$loopback_ip" ]
	then
	  # Append s3 hostname to end of loopback_ip
	  sed -i "/^$loopback_ip/s/$/ $HOSTNAME/" $file
	else
	  # Append s3 hostname to end of /etc/hosts
	  s3_host_entry="$s3_host_ip   $HOSTNAME"
	  echo "$s3_host_entry" >> $file
	fi
}

step_4 () {
	# Try to install rpm pkgs from s3 preqs file
	json_prereqs_file="/opt/seagate/cortx/s3/mini-prov/s3setup_prereqs.json"
	rpms_list=$(jq '.post_install.rpms' $json_prereqs_file)
	rpms_list=$(echo "$rpms_list" | tr -d '[]"\r\n')
	IFS=', ' read -r -a rpms <<< "$rpms_list"
	if [ ${#rpms[@]} -eq 0 ];then
		die_with_error "jq '.post_install.rpms' $json_prereqs_file failed"
	fi
	if [ "${rpms[0]}" == "null" ];then
		die_with_error "jq '.post_install.rpms' $json_prereqs_file failed"
	fi

	for rpm in "${rpms[@]}" ; do
		echo "Trying to install rpm: $rpm"
		yum install "$rpm" -y --nogpgcheck || die_with_error "$rpm install failed!"
	done
}

step_5 () {
	# update_haproxy_cfg
	cfg_file="/etc/haproxy/haproxy.cfg"
	if [ ! -e $cfg_file ];then
		die_with_error "$cfg_file does not exists!"
	fi
	sed -e '/ssl-default-bind-ciphers PROFILE=SYSTEM/ s/^#*/#/' -i $cfg_file
	sed -e '/ssl-default-server-ciphers PROFILE=SYSTEM/ s/^#*/#/' -i $cfg_file
}

step_6 () {
	# TODO: Kafka (Future, needed when rabbit-mq dependency is removed)
	echo "TODO: implement when kafka replaces rabbit-mq dependency"
	"sh scripts/kafka/install-kafka.sh -c 1 -i $hostname"
	"sh scripts/kafka/create-topic.sh -c 1 -i $hostname"
}

step_7 () {
	services_list=$(jq '.post_install.services' $json_prereqs_file)
	services_list=$(echo "$services_list" | tr -d '[]"\r\n')
	IFS=', ' read -r -a services <<< "$services_list"
	if [ ${#services[@]} -eq 0 ];then
		die_with_error "jq '.post_install.services' $json_prereqs_file failed"
	fi
	if [ "${services[0]}" == "null" ];then
		die_with_error "jq '.post_install.services' $json_prereqs_file failed"
	fi
	# services=("$@")
	for service in "${services[@]}" ; do
		echo "Trying to restart service: $service"
		systemctl restart "$service" || die_with_error "$service failed to start!"
	done
}

step_8 () {
	# prepare the confstore related configs
	myhostname=$(hostname -f)
	myip=$(/sbin/ip -o -4 addr list eth2 | awk '{print $4}' | cut -d/ -f1)
	key=$(s3cipher generate_key --const_key='openldap')
	iampasswd=$(s3cipher encrypt --data="ldapadmin" --key="$key")
	rootdnpasswd=$(s3cipher encrypt --data="seagate" --key="$key")
	s3_conf_file="/tmp/s3_confstore.json"

	s3instances=11
	read -p "Enter number of s3 instances (default is $s3instances): " s3instances
	[ "$s3instances" == "" ] && s3instances=11

	public_ip="127.0.0.1"
	read -p "Enter public IP (default is $public_ip): " public_ip
	[ "$public_ip" == "" ] && public_ip="127.0.0.1"

	private_ip="myip"
	read -p "Enter private IP (default is $myip): " private_ip
	[ "$private_ip" == "" ] && private_ip="$myip"

	if [ -e $s3_conf_file ]; then
	  mv $s3_conf_file $s3_conf_file.bak
	fi

	if [ ! -e "/etc/machine-id" ]; then
	  die_with_error "/etc/machine-id not found"
	fi

	machineid=$(</etc/machine-id)

	if [ ! -e "/opt/seagate/cortx/s3/s3backgrounddelete/s3_cluster.yaml" ]; then
	  die_with_error "/opt/seagate/cortx/s3/s3backgrounddelete/s3_cluster.yaml not found"
	fi

	clustreid=$(grep cluster_id /opt/seagate/cortx/s3/s3backgrounddelete/s3_cluster.yaml | tr ":" "\n" | tail -1 | tr "#" "\n" | head -1 | xargs)

	tee -a $s3_conf_file << END
{
	"openldap":
	{
		"root":
		{
			"user": "admin",
			"secret": "$rootdnpasswd"

		},
		"sgiam":
		{
			"user": "sgiamadmin",
			"secret": "$iampasswd"

		}

	},
	"cluster":
	{
		"env_type": "VM",
		"cluster_id": "$clustreid",
		"cluster_ip": "127.0.0.1",
		"dns_servers": [ "8.8.8.8"  ],
		"mgmt_vip": "127.0.0.1",
		"server_nodes":
		{
			"$machineid": "srvnode_1"

		},
		"srvnode_1":
		{
			"s3_instances": "$s3instances",
			"machine_id": "$machineid",
			"hostname": "$myhostname",
			"network":
			{
				"data":
				{
					"gateway": "255.255.255.0",
					"interfaces": [ "eth1", "eth2"  ],
					"netmask": "255.255.255.0",
					"public_ip": "$public_ip",
					"private_ip": "$private_ip",
					"roaming_ip": "127.0.0.1"
				}
			}
		}
	}
}
END

	mkdir -p /etc/ssl/stx/ || die_with_error "mkdir -p /etc/ssl/stx/ failed"
	if [ -e /etc/ssl/stx/stx.pem ]; then
		return
	fi
	tee -a /etc/ssl/stx/stx.pem << END
-----BEGIN CERTIFICATE-----
MIIDozCCAougAwIBAgIBATANBgkqhkiG9w0BAQsFADBJMQswCQYDVQQGEwJJTjEN
MAsGA1UEBwwEUHVuZTEVMBMGA1UECgwMU2VhZ2F0ZSBUZWNoMRQwEgYDVQQDDAtz
ZWFnYXRlLmNvbTAeFw0yMDAzMjAwNTMwMDlaFw0yMTAzMjAwNTMwMDlaMEkxCzAJ
BgNVBAYTAklOMQ0wCwYDVQQHDARQdW5lMRUwEwYDVQQKDAxTZWFnYXRlIFRlY2gx
FDASBgNVBAMMC3NlYWdhdGUuY29tMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIB
CgKCAQEArxX7Svopjw6mL8ySVqtusgO17ZBkwTJQGZbyP+AMyXm2DhKNxBHcPNWT
xJqMu7jAD3G/PMUip+ILWACErzGtkvG5zH0DA0C48CTIfoR7bYY/KR+eWDefzI9d
qk3lXqN1H2HzSwK9Kn/ols1s5D1pb1EjsDq1sQTAFH6CYeZl8Aqe7qahgH1Uub6s
5oXxg653g4Pof+PEzCI3p5nP9CWb+14lg/yGEB7BWhmZ7QmKOyn5onMT+xcRpC6H
TiCsXjl5YaVMFYBRNMkc15tBeIobIeUSCvDiuVkqYGtdB4INU9I67XBpmcjMoLi0
eFtqKaOuHDqZWqBzpi3xx3hSv/cucQIDAQABo4GVMIGSMDUGA1UdEQQuMCyCC3Nl
YWdhdGUuY29tgg0qLnNlYWdhdGUuY29tggkxMjcuMC4wLjGCAzo6MTALBgNVHQ8E
BAMCBaAwHQYDVR0OBBYEFEbLrob/NQsyHmFCz1gvdiKlbgViMB8GA1UdIwQYMBaA
FK1kOf3UtxzotWL9NVlNalILewWPMAwGA1UdEwQFMAMBAf8wDQYJKoZIhvcNAQEL
BQADggEBAHsQlTvXVcugqkqAs6jHG97cqjR0/AY6DIHJ+oezNDznthQTDyqJ6Ei9
KJgQ6YBCi/5xf/zkeTXPlIJRpsd8JYpbWQVekgmVulg4t9MDrcQFZ7GvHRpZpAwu
iKMsZXouPZV5KmKvTss0ZdbU6j/kFylfW1Kh32ZJrRN7ACf2P5soIW676vSAfkRq
EG9WjGbwVkAF1hZdK1U2SRY+/M6qLvfn8Y1pCmIWW1756E/mTUIybXUhlVHNm073
b+TsHB71ssHk3RpXcAyXd5MUH1BxFlWy66F3N89dQj4ht3y/eXa70Sm9u4AMHoT1
7aW4YzZL5/43nT+mDnRqixWx/jzLs8M=
-----END CERTIFICATE-----
-----BEGIN RSA PRIVATE KEY-----
MIIEpAIBAAKCAQEArxX7Svopjw6mL8ySVqtusgO17ZBkwTJQGZbyP+AMyXm2DhKN
xBHcPNWTxJqMu7jAD3G/PMUip+ILWACErzGtkvG5zH0DA0C48CTIfoR7bYY/KR+e
WDefzI9dqk3lXqN1H2HzSwK9Kn/ols1s5D1pb1EjsDq1sQTAFH6CYeZl8Aqe7qah
gH1Uub6s5oXxg653g4Pof+PEzCI3p5nP9CWb+14lg/yGEB7BWhmZ7QmKOyn5onMT
+xcRpC6HTiCsXjl5YaVMFYBRNMkc15tBeIobIeUSCvDiuVkqYGtdB4INU9I67XBp
mcjMoLi0eFtqKaOuHDqZWqBzpi3xx3hSv/cucQIDAQABAoIBADijwsxpiyI1Wfui
kUCqar/5xVPZ305EiXcNxsZ1I43V6tg4llX0dSvU5921JYvg43jbkkMFfwWScZsB
Z+sJBh7ARhvp4RyfRnShYZ7UGt2+jRYvnVjqfa5+Po3Gb0ojVNNXK457j1h9Um/e
eriHSWFyfToYdAiVAdJfbqxfDWEOdQPv0cLQ14q/neHDzwchuTzpRglKBFHOBGiB
EAD8kfKQeRY8Y606VafiiS468+n5kicCX7mbhYV8eKR7B6smhav8ld4Syg3bEovx
RTqL0c9yyyDduXNQRpwXQ5sAuaWaJwtF1A0NW6m32BldZdPOerGYP5L9eHlcGyGU
sZVJGXECgYEA5yXj3MT0n4XZblFhbxWfjzi0LLNAMycaxydmQR8gwwzknNpWlvP1
gb1fiOkVHz95rmR99np/TbRkDf2pvkmhwJJLOXreAy/HDc75WO8IWh3AC4XGvUri
ZiNm0r7hOgMZFuvOIcCCr+Ivl0eUsMqBR9sngQhQF3caHBWATLBXh+0CgYEAwekJ
/WqjECG1ccN0Hy/gH/ATEFplzGc82zNNgDJc8lj7fPczFB/LqEZaTLRa+vosZfHX
0ArDUWf49fChSXYMAV+9jCd0peSV3NzBJOCsy/PrtgVag7MmNz9TWDriC9WAUgfg
0AsBZb+LNIdfGFBpTy2cTNIBZY5agNMRecA8KBUCgYBx6TlO7f0DtGm/tPlITiaw
5Sfds4Sa8NWAeckppJZ3qR3ssqjjgVxm0JWJGhyfpp9nsvxkgF/GQnTgdDa3oP/G
sBHEROmuNlhpVKuLCVlbV7fxtb6IQKQr45xjlU/XT/mIUzLlbUK6PMRpUAxVx6ZI
bBcevqMBvV8voeT1Zh3szQKBgQC9hPt7kB7BZIDHKKW02YDvFiA7ym5WQcyL3O9x
TUfkoS1i7OQiVhUhaWlWMKv/QkXkeWNZdTuCs+Dy1vV8LAD90soaUnHCtc/25ldr
qJ+aUtNcuozFzXGba6wUvrAxqsDY69RA4ZDDFluCwpAh3m2eslBiEJrG3EP+Rsx8
t3LUJQKBgQDORp7oDFJhD+u4Akb6V8GLGgXZKPAPV9jkz+jvC9esQxarFP4pku7u
yjmwy6Eep2BoKjn4DgiWPEcuOhMChav9bqilFtgq+zpgwGCYl+Vy5hUXpCfLsDiQ
ZnGFoVwgsRF7FyXhmIEX89HLrK7DqIoYtLV3ZH7m96TFt9l2DSCU2A==
-----END RSA PRIVATE KEY-----
END

}

step_9 () {
	# perform s3 mini-proviosner setup
	bash -x /opt/seagate/cortx/s3/bin/s3_setup post_install --config "json://$s3_conf_file" || die_with_error "s3:post_install failed!"
	echo "s3:post_install passed!"

	bash -x /opt/seagate/cortx/s3/bin/s3_setup config --config "json://$s3_conf_file" || die_with_error "s3:config failed"
	echo "s3:config passed"

	bash -x /opt/seagate/cortx/s3/bin/s3_setup init --config "json://$s3_conf_file" || die_with_error "s3:init failed"
	echo "s3:init passed"


	if [ "$1" = "true" ];then
		bash -x /opt/seagate/cortx/s3/bin/s3_setup cleanup --config "json://$s3_conf_file" &> /dev/null || die_with_error "s3:cleanup failed!"
		echo "s3:cleanup passed!"
	fi
}

step_10 () {
	# perform single node IO setup for cluster
	hctl status && die_with_error "IO setup already performed"
	yum install -y --nogpgcheck cortx-motr || die_with_error "cortx-motr install failed"
	yum-config-manager --add-repo https://rpm.releases.hashicorp.com/RHEL/hashicorp.repo || die_with_error "yum-config-manager https://rpm.releases.hashicorp.com/RHEL/hashicorp.repo failed"
	yum install -y --nogpgcheck cortx-hare || die_with_error "cortx-hare install failed"
	hostname > /var/lib/hare/node-name || die_with_error "/var/lib/hare/node-name could not be updated"
	m0setup || die_with_error "cortx-hare install failed" 
	echo "options lnet networks=tcp(eth0) config_on_load=1" > /etc/modprobe.d/lnet.conf || die_with_error "lnet.conf couldn't be updated"
	service lnet start || die_with_error "lnet service couldn't be started"
	cdf_conf_file="/tmp/singlenode.yaml"

	if [ -e $cdf_conf_file ]; then
	  mv $cdf_conf_file $cdf_conf_file.bak
	fi

	tee -a $cdf_conf_file << END
nodes:
 - hostname: $HOSTNAME
   data_iface: eth0
   data_iface_type: tcp
   m0_servers:
    - runs_confd: true
      io_disks:
       data: []
    - io_disks:
       data:
        - /dev/loop1
        - /dev/loop2
        - /dev/loop3
        - /dev/loop4
   m0_clients:
    s3: 1
    other: 2
pools:
 - name: the pool
   type: sns
   data_units: 2
   parity_units: 1
END
	hctl bootstrap --mkfs $cdf_conf_file || die_with_error "hctl bootstrap failed"
	hctl status || die_with_error "hctl status failed"
	systemctl restart s3authserver.service || die_with_error " systemctl restart s3authserver.service failed"
	systemctl start s3backgroundproducer || die_with_error "systemctl start s3backgroundproducer failed"
	systemctl start s3backgroundconsumer || die_with_error "systemctl start s3backgroundconsumer  failed"
	yum install --nogpgcheck cortx-s3iamcli s3cmd -y || die_with_error "cortx-s3iamcli s3cmd could not be installed"
	bash -x /opt/seagate/cortx/s3/bin/s3_setup test --config "json:///tmp/s3_confstore.json" || die_with_error "S3 sanity on IO failed"
}

prerequisite=false
s3setup=false
iosetup=false
cleanup=false
addr=""
set -a

USAGE="USAGE: bash $(basename "$0")
	Run S3 pre-requisites and mini-provisioner steps automated from https://github.com/Seagate/cortx-s3server/wiki/S3server-provisioning-on-single-node-VM-cluster:-Manual
	[--help]
	{ -a HostIpv4 }
	{ -s }
	{ -i }
	{ -c }
where:
	-h  Display help
	-a  Use the provided ipv4 address for host \"$HOSTNAME\" and update /etc/hosts
	-p  Perform only prerequisite steps needed before S3 mini-provisioner single node setup
	-s  Perform S3 mini-provisioner single node setup with prerequisite steps but without IO
	-i  Perform S3 mini-provisioner single node setup with prerequisite steps and IO
	-c  Perform S3 mini-provisioner single node setup with prerequisite steps but without IO and cleanup
Note: options -i and -c can not be used together."

# Start of the script main
# Process command line args
OPTIND=1
while getopts "h?psica:" o; do
	case "${o}" in
		h|\?)
			echo "$USAGE"
			exit 0
			;;
		p)
			prerequisite=true
			;;
		s)
			prerequisite=true
			s3setup=true
			;;
		i)
			prerequisite=true
			s3setup=true
			iosetup=true
			;;
		c)
			prerequisite=true
			s3setup=true
			cleanup=true
			;;
		a)
			addr=${OPTARG}
			;;
		*)
			die_with_error "$USAGE"
			;;
		esac
done
shift $((OPTIND-1))

echo "s3 mini provisioner prerequisite steps (-p): $prerequisite"
echo "s3 mini provisioner setup (-s): $s3setup"
echo "s3 mini provisioner IO setup (-i): $iosetup"
echo "s3 mini provisioner setup cleanup (-c): $cleanup"
echo "user provided IP address (-a): $addr"

grep "cortx_iso" /etc/yum.repos.d/* || die_with_error "Please add cortx_iso repo and rerun"
grep "s3server_uploads" /etc/yum.repos.d/* || yum-config-manager --add-repo http://cortx-storage.colo.seagate.com/releases/cortx/uploads/centos/centos-7.8.2003/s3server_uploads/ || die_with_error "Please add s3server_uploads repo and rerun"

if [ "$iosetup" = true ];then
	if [ "$cleanup" = "true" ];then
		die_with_error "Error: options -i and -c can not be used together."
	fi
	grep "lustre" /etc/yum.repos.d/* || die_with_error "Please add lustre repo and rerun"
fi

command -v jq &> /dev/null || die_with_error "jq could not be found!"
command -v yum &> /dev/null || die_with_error "yum could not be found!"
command -v systemctl &> /dev/null || die_with_error "systemctl could not be found!"

if [ "$prerequisite" = "true" ];then
	step_2
	step_3 "$addr"
	step_4
	step_5
	step_6
	step_7
	step_8
fi

if [ "$s3setup" = "true" ];then
	step_9 "$cleanup"
fi

if [ "$iosetup" = true ];then
		step_10
fi	

echo "All steps passed!"
