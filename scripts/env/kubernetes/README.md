
This page provides steps to set up s3 in k8s env using automated scripts.

# Intro

Go to the terminal on the node where you want to deploy k8s, login as root,
and execute commands listed below.

Simply copy the entire commands list from a box, and paste into the
console.


# Download needed files to your node

## Step 1 - define repo/branch as environment variables

NOTE -- this is the only box which you may need to edit before running.
Modify the repo URL and branch if needed.  (Modifications should only be
needed if you're a developer and want to test your modifications to deploy
scripts before merge.)

```
REPO_URL=https://github.com/Seagate/cortx-s3server.git
BRANCH_NAME=k8s-automation
```

## Step 2

```
mkdir -p /var/data/cortx
cd /var/data/cortx
git clone "$REPO_URL"
cd cortx-s3server
git checkout "$BRANCH_NAME"
```

# Provide deployment parameters

## Step 1 - user-defined parameters

Commands below will open config file in editor.  Update as needed, and
save.

```
cd /var/data/cortx/cortx-s3server/scripts/env/kubernetes
cp config.sh.template config.sh
vi config.sh
```

# Step 2 - auto-detected environment parameters

```
cd /var/data/cortx/cortx-s3server/scripts/env/kubernetes
./sh/create-env.sh
```

# Run deployment

```
cd /var/data/cortx/cortx-s3server/scripts/env/kubernetes
./sh/00-run-all-steps.sh
```
