#!/bin/bash
#
# Copyright (c) 2021 Seagate Technology LLC and/or its Affiliates
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

set -euo pipefail # exit on failures

source ./config.sh
source ./env.sh
source ./sh/functions.sh

set -x # print each statement before execution

add_separator INSTALLING KUBERNETES ON THE NODE.


yum install -y yum-utils
if [ ! -f /etc/yum.repos.d/docker-ce.repo ]; then
  yum-config-manager --add-repo https://download.docker.com/linux/centos/docker-ce.repo
fi
yum install -y docker-ce docker-ce-cli containerd.io
if ! systemctl status docker; then
  systemctl start docker
fi

# authorize with docker.com (to fix rate limiting)
if [ -n "${DOCKER_USER_NAME}${DOCKER_PASSWORD}" ]; then
  docker login -u "$DOCKER_USER_NAME" -p "$DOCKER_PASSWORD"
fi

# check if this has already been executed
if kubectl get node "$HOST_FQDN" --show-labels | grep node-name="$NODE_NAME"; then
  add_separator "K8S INSTALLATION FOR THIS NODE ALREADY DONE BEFORE"
  exit 0
fi

# pre-install centos 7.9 image, if available.  Will speed up things later.
if wget http://cortx-storage.colo.seagate.com/releases/cortx/images/centos-7.9.2009.tar; then
  docker image load -i centos-7.9.2009.tar || true
    # ok to ignore error; will be properly loaded later anyway.
fi

# install kubernetes
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

set +x
actual=$( sysctl --system \
  | grep k8s.conf -A3 \
  | grep -v 'Applying /etc/sysctl.d/k8s.conf' \
  | grep -v 'net.bridge.bridge-nf-call-ip6tables = 1' \
  | grep -v 'net.bridge.bridge-nf-call-iptables = 1' \
  | wc -l)
if [ "$actual" -ne 1 ]; then
  echo 'Rules in /etc/sysctl.d/k8s.conf failed to apply. Exiting.'
  add_separator FAILED
  false
fi
set -x

if [ "$(getenforce)" != 'Disabled' ]; then
  setenforce 0
  sed -i 's/^SELINUX=enforcing$/SELINUX=permissive/' /etc/selinux/config
fi

sed -i '/swap/d' /etc/fstab
swapoff -a

set +x
if [ "$(free | grep ^Swap | awk '{print $2+$3+$4}')" -ne 0 ]; then
  add_separator 'FAILURE: Failed to disable swap'
  set -x
  free
  false
fi
set -x

systemctl enable docker.service
cat <<EOF > /etc/docker/daemon.json
{
  "exec-opts": ["native.cgroupdriver=systemd"]
}
EOF
sudo systemctl restart docker

if [ "$THIS_IS_PRIMARY_K8S_NODE" = yes ]; then
  kubeadm init --pod-network-cidr=192.168.0.0/16 2>&1 | tee kubeadm-init.log
  cat kubeadm-init.log
  mkdir -p $HOME/.kube
  # This is a workaround for SSC machines where root cannot write to /home
  (sudo cat /etc/kubernetes/admin.conf) > ~/.kube/config
  sudo chown $(id -u):$(id -g) $HOME/.kube/config

  # save kubeadm join command to separate file
  cat kubeadm-init.log \
    | grep 'kubeadm join\|--token\|--discovery-token-ca-cert-hash' \
    > kubeadm-join-cmd.sh

  curl https://docs.projectcalico.org/manifests/calico.yaml -O
  # download images using docker -- 'kubectl init' is not able to apply user
  # credentials, and so is suffering from rate limits.
  pull_images_for_pod calico.yaml
  kubectl apply -f calico.yaml
fi


if [ "$THIS_IS_PRIMARY_K8S_NODE" = yes ]; then
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

set +x
add_separator Checking kube-system PODs status
while [ -n "$(kubectl get pods -n kube-system | grep -v NAME | safe_grep -v Running)" ]; do
  echo
  kubectl get pods -n kube-system | grep -v Running
  echo
  echo Some of the pods are not yet in Running state, re-checking ...
  echo '(hit CTRL-C if it is taking too long)'
  sleep 5
done
set -x


set +x
add_separator Checking node status
while [ -n "$(kubectl get nodes | grep -v NAME | safe_grep -v Ready)" ]; do
  echo
  kubectl get nodes | grep -v Running
  echo
  echo Some of the nodes are not yet in Ready state, re-checking ...
  echo '(hit CTRL-C if it is taking too long)'
  sleep 5
done
set -x

kubectl label node "$HOST_FQDN" node-name="$NODE_NAME"

add_separator SUCCESSFULLY INSTALLED KUBERNETES.
