
This page provides steps to set up s3 in k8s env using automated scripts.

# Intro

Go to the terminal on the node where you want to deploy k8s, login as root,
and execute commands listed below.


# Download needed files to your node

Note - Modify the repo URL and branch if needed.

```
mkdir -p /var/data/cortx
cd /var/data/cortx
git clone https://github.com/Seagate/cortx-s3server.git
cd cortx-s3server
git checkout k8s-automation
```

# Config

Commands below will open config file in editor.  Update as needed, and save.

```
cd /var/data/cortx/cortx-s3server/scripts/env/kubernetes
cp config.sh.template config.sh
vim config.sh
```

# Run

```
cd /var/data/cortx/cortx-s3server/scripts/env/kubernetes
./sh/install-k8s.sh
```
