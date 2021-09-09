#!/bin/bash
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

source ./config.sh

source ./sh/functions.sh

set -e # exit immediatly on errors
set -x # print each statement before execution

yum install -y yum-utils
yum-config-manager --add-repo https://download.docker.com/linux/centos/docker-ce.repo
yum install -y docker-ce docker-ce-cli containerd.io
systemctl start docker

# self-check

sudo docker run hello-world

self_check "Do you see success message above?  (Something like 'Hello from Docker!')"

cat <<EOF > /etc/yum.repos.d/kubernetes.repo
[kubernetes]
name=Kubernetes
baseurl=https://packages.cloud.google.com/yum/repos/kubernetes-el7-x86_64
enabled=1
gpgcheck=1
repo_gpgcheck=1
gpgkey=https://packages.cloud.google.com/yum/doc/yum-key.gpg https://packages.cloud.google.com/yum/doc/rpm-package-key.gpg
EOF

yum install -y kubelet kubeadm kubectl
systemctl enable kubelet
systemctl start kubelet

cat <<EOF | sudo tee /etc/modules-load.d/k8s.conf
br_netfilter
EOF

cat <<EOF > /etc/sysctl.d/k8s.conf
net.bridge.bridge-nf-call-ip6tables = 1
net.bridge.bridge-nf-call-iptables = 1
EOF

add_separator listing all rules
sysctl --system

add_separator listing k8s rules
sysctl --system | grep k8s.conf -A3

set +x
self_check "Do you see the following lines in above output?

Applying /etc/sysctl.d/k8s.conf ...
net.bridge.bridge-nf-call-ip6tables = 1
net.bridge.bridge-nf-call-iptables = 1

"
set -x

if [ `getenforce` != 'Disabled' ]; then
  setenforce 0
  sed -i 's/^SELINUX=enforcing$/SELINUX=permissive/' /etc/selinux/config
fi

sed -i '/swap/d' /etc/fstab
swapoff -a

free -mh

self_check "Is swap showing 0 bytes in output above?"

systemctl enable docker.service
cat <<EOF > /etc/docker/daemon.json
{
  "exec-opts": ["native.cgroupdriver=systemd"]
}
EOF
sudo systemctl restart docker

if [ "$THIS_IS_MASTER_K8S_NODE" -eq 1 ]; then
  kubeadm init --pod-network-cidr=192.168.0.0/16
  mkdir -p $HOME/.kube
  # This is a workaround for SSC machines where root cannot write to /home
  (sudo cat /etc/kubernetes/admin.conf) > ~/.kube/config
  sudo chown $(id -u):$(id -g) $HOME/.kube/config

  set +x
  add_separator Save important data
  echo "
  
Copy 'kubeadm join' command from above outputs.  Should look like the following:

kubeadm join 10.230.244.241:6443 --token kxr1t6.iql7ih3uq2lr0e7l \\
          --discovery-token-ca-cert-hash sha256:d221dd8ccd93871d4226ad60bda697f9cb98af8e0c5c0ece523a5c95a704ea27

Then paste the command back to this terminal below and hit CTRL-D:"
  cat >kubeadm-join-cmd.sh
  set -x

  curl https://docs.projectcalico.org/manifests/calico.yaml -O
  kubectl apply -f calico.yaml
fi


if [ "$THIS_IS_MASTER_K8S_NODE" -eq 1 ]; then
  kubectl taint nodes --all node-role.kubernetes.io/master-
else
  set +x
  add_separator Joining the cluster
  echo "

On the master k8s node, run:

cat /var/data/cortx/cortx-s3server/scripts/env/kubernetes/kubeadm-join-cmd.sh

copy the output and paste in this terminal below.  Then hit CTRL-D:"
  cat | /bin/sh
  set -x
fi

while true; do
  set -x
  kubectl get pods -n kube-system
  set +x
  if [ "$THIS_IS_MASTER_K8S_NODE" -eq 1 ]; then
    if self_check 'The above output should look similar to this example below:

NAME                                                            READY   STATUS    RESTARTS   AGE
calico-kube-controllers-58497c65d5-9vj4c                        1/1     Running   0          76m
calico-node-p9xlb                                               1/1     Running   0          76m
coredns-78fcd69978-bn5wb                                        1/1     Running   0          76m
coredns-78fcd69978-httl2                                        1/1     Running   0          76m
etcd-ssc-vm-g3-rhev4-0880.colo.seagate.com                      1/1     Running   0          77m
kube-apiserver-ssc-vm-g3-rhev4-0880.colo.seagate.com            1/1     Running   0          77m
kube-controller-manager-ssc-vm-g3-rhev4-0880.colo.seagate.com   1/1     Running   0          77m
kube-proxy-r9h9s                                                1/1     Running   0          76m
kube-scheduler-ssc-vm-g3-rhev4-0880.colo.seagate.com            1/1     Running   0          77m

Does the output above match? (all must be Running)'; then
      break
    fi
  else
    if self_check 'The above output should look similar to this example below:

[root@sm6-r1 ~]# kubectl get pods -n kube-system
NAME                                             READY   STATUS    RESTARTS   AGE
calico-kube-controllers-78d6f96c7b-xz7jd         1/1     Running   0          6m52s
calico-node-4s7fc                                1/1     Running   0          71s
calico-node-jcqxg                                1/1     Running   0          71s
calico-node-kxmm2                                1/1     Running   0          71s
coredns-558bd4d5db-2tlnc                         1/1     Running   0          17m
coredns-558bd4d5db-qwk5p                         1/1     Running   0          17m
etcd-sm6-r1.pun.seagate.com                      1/1     Running   0          18m
kube-apiserver-sm6-r1.pun.seagate.com            1/1     Running   0          18m
kube-controller-manager-sm6-r1.pun.seagate.com   1/1     Running   0          18m
kube-proxy-ctdhz                                 1/1     Running   0          17m
kube-proxy-h765n                                 1/1     Running   0          14m
kube-proxy-n2l8j                                 1/1     Running   0          14m
kube-scheduler-sm6-r1.pun.seagate.com            1/1     Running   0          18m

There must be one calico-node and one kube-proxy per node in cluster.

Does the output above match? (all must be Running)'; then
      break
    fi
  fi
  if ! self_check 'Retry again?  (Recommendation is retry at least a few times.  Typically it completes within a minute.)'; then
    exit 1
  fi
done
set -x

while true; do
  set -x
  add_separator Checking nodes status
  kubectl get nodes
  set +x
  if self_check "Is it showing STATUS=Ready?"; then
    break
  fi
  if ! self_check 'Retry again?  (Recommendation is retry at least a few times.  Typically it completes within a minute.)'; then
    exit 1
  fi
done
set -x

add_separator "Adding node label"
hostname=`hostname`
set +x
echo
read -p "Input node FQDN (or hit enter if default value is correct) [$hostname] " var
if [ -n "$var" ]; then
  hostname="$var"
fi
set -x

kubectl label node "$hostname" node-name="$NODE_LABEL"

add_separator SUCCESSFULLY INSTALLED KUBERNETES.
