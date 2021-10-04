
This page provides steps to set up s3 in k8s env using automated scripts.

# Intro

* Create VM with OS CentOS 7.9, at least 8 CPUs, 12 disks 50GB each, 16 GB
  RAM.
* Open terminal.
* Switch to a `root` user.
* Execute execute commands listed below.
  * Simply copy the entire commands list from each box, and paste into the
    console.

NOTE: if you run this script many times during the day -- you will probably
hit the docker registry rate limiting (docker will not allow you to
download images).  To solve that, sign up on https://hub.docker.com, and
provide user and password in `config.sh` step below.

# 1. Download needed files to your node

## 1.1. Define repo/branch as environment variables

NOTE -- this is the only box which you may need to edit before running.
Modify the repo URL and branch if needed.  (Modifications should only be
needed if you're a developer and want to test your modifications to deploy
scripts before merge.)

```sh
REPO_URL=https://github.com/Seagate/cortx-s3server.git
BRANCH_NAME=k8s-automation
```

## 1.2. Download

```bash
mkdir -p /var/data/cortx
cd /var/data/cortx
git clone "$REPO_URL"
cd cortx-s3server
git checkout "$BRANCH_NAME"
```

# 2. Provide deployment parameters

## 2.1. User-defined parameters

Commands below will open config file in editor.  Update as needed, and
save.

```
cd /var/data/cortx/cortx-s3server/scripts/env/kubernetes
cp config.sh.template config.sh
vim config.sh
```

# 2.2. Auto-detected environment parameters

```
cd /var/data/cortx/cortx-s3server/scripts/env/kubernetes
./sh/create-env.sh
```

# 3. Run deployment

```
cd /var/data/cortx/cortx-s3server/scripts/env/kubernetes
./sh/000-run-all-steps.sh
```
