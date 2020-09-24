# Contribute to S3 Server

- [1.0 Prerequisites](#10-Prerequisites)
- [1.2 Set up Git on your Development Box](#12-Set-Up-Git-on-your-Development-Box)
- [1.3 Submit your changes](#13-Submit-your-Changes)
   * [1.3.1 Clone the cortx-s3server repository](#131-Clone-the-cortx-s3server-repository)
   * [1.3.2 Code Commits](#132-Code-commits)
   * [1.3.3 Create a Pull Request](#133-Create-a-Pull-Request)
- [FAQs](FAQs)

Contributing to the S3 Server repository is a three-step process where you'll need to:

1. [Clone the cortx-s3server repository](#131-Clone-the-cortx-s3server-repository)
2. [Commit your Code](#132-Code-commits)
3. [Create a Pull Request](#133-Create-a-Pull-Request)

## 1.0 Prerequisites

<details>
  <summary>Before you begin</summary>
    <p>

Before you set up your GitHub, you'll need to
1. Generate the SSH key on your development box using:

    ```shell

    $ ssh-keygen -o -t rsa -b 4096 -C "your email-address"
    ```
2. Add the SSH key to your GitHub Account:
   1. Copy the public key: `id_rsa.pub`. By default, your public key is located at `/root/.ssh/id_rsa.pub`
   2. Navigate to [GitHub SSH key settings](https://github.com/settings/keys) on your GitHub account.

      :page_with_curl:**Note:** Ensure that you've set your Email ID as the Primary Email Address associated with your GitHub Account. 

   3. Paste the SSH key you generated in Step 1 and select *Add-ssh-key*.
   4. [Create a Personal Access Token or PAT](https://help.github.com/en/github/authenticating-to-github/creating-a-personal-access-token).

   </p>
    </details>

## 1.2 Set Up Git on your Development Box

<details>
  <summary>Before you begin</summary>
    <p>

1. Update Git to the latest version. If you're on an older version, you'll see errors in your commit hooks that look like this:

    `$ git commit`

    ```shell

    git: 'interpret-trailers' is not a git command.
    See 'git --help'
    cannot insert change-id line in .git/COMMIT_EDITMSG
    ```

2. Install Fix for CentOS 7.x by using:

   `$ yum remove git`

    Download the [RPM file from here](https://packages.endpoint.com/rhel/7/os/x86_64/endpoint-repo-1.7-1.x86_64.rpm) and run the following commands:

    ```shell
   
      $ yum -y install
      $ yum -y install git
     ```
 </p>
 </details>

Once you've installed the prerequisites, follow these steps to set up Git on your Development Box:

1. Install git-clang-format using: `$ yum install git-clang-format`

2. Set up git config options using:

   ```shell

   $ git config --global user.name ‘Your Name’
   $ git config --global user.email ‘Your.Name@Domain_Name’
   $ git config --global color.ui auto
   $ git config --global credential.helper cache
   ```
## 1.3. Submit your Changes

Before you can work on a GitHub feature, you'll need to clone the cortx-s3server repository.

### 1.3.1 Clone the cortx-s3server repository

You'll need to **Fork** the cortx-s3server repository to clone it into your private GitHub repository. Follow these steps to clone the repository to your gitHub account:
1. Navigate to the 'cortx-s3server' repository homepage on GitHub.
2. Click **Fork**
3. Run the following commands in Shell:

   `git clone --recursive https://github.com/Seagate/cortx-s3server.git -b main`

4. Check out to the “main” branch using:

   `$ git checkout main`

   `$ git checkout -b "your-local-branch-name"`

### 1.3.2 Code Commits

You can make changes to the code and save them in your files.

1. Use the command below to add files that need to be pushed to the git staging area:

    `$ git add foo/somefile.cc`

2. To commit your code changes use:

   `$ git commit -s -m 'comment'` - enter your GitHub Account ID and an appropriate Feature or Change description in comment.


3. Check out your git log to view the details of your commit and verify the author name using: `$ git log`

    :page_with_curl:**Note:** If you need to change the author name for your commit, refer to the GitHub article on [Changing author info](https://docs.github.com/en/github/using-git/changing-author-info).

4. To Push your changes to GitHub, use: `$ git push origin 'your-local-branch-name'`

   Your output will look like the Sample Output below:

   ```shell

   Enumerating objects: 4, done.
   Counting objects: 100% (4/4), done.
   Delta compression using up to 2 threads
   Compressing objects: 100% (2/2), done.
   Writing objects: 100% (3/3), 332 bytes | 332.00 KiB/s, done.
   Total 3 (delta 1), reused 0 (delta 0)
   remote: Resolving deltas: 100% (1/1), completed with 1 local object.
   remote:
   remote: Create a pull request for 'your-local-branch-name' on GitHub by visiting:
   remote: https://github.com/<your-GitHub-Id>/cortx-s3server/pull/new/<your-local-branch-name>
   remote: To github.com:<your-GitHub-Id>/cortx-s3server.git
   * [new branch] <your-local-branch-name> -> <your-local-branch-name>
   ```

### 1.3.3 Create a Pull Request


1. Once you Push changes to GitHub, the output will display a URL for creating a Pull Request, as shown in the sample code above.
:page_with_curl:**Note:** To resolve conflicts, follow the troubleshooting steps mentioned in git error messages.
2. You'll be redirected to GitHib remote.
3. Select **main** from the Branches/Tags drop-down list.
4. Click **Create pull request** to create the pull request.
5. Add reviewers to your pull request to review and provide feedback on your changes.

## FAQs

**Q.** How do I rebase my local branch to the latest main branch?

**A** Follow the steps mentioned below:

```shell

$ git pull origin main
$ git submodule update --init --recursive
$ git checkout 'your-local-branch'
$ git pull origin 'your-remote-branch-name'
$ git submodule update --init --recursive
$ git rebase origin/main
```

## You're All Set & You're Awesome!

We thank you for stopping by to check out the CORTX Community. We are fully dedicated to our mission to build open source technologies that help the world save unlimited data and solve challenging data problems. Join our mission to help reinvent a data-driven world. 

### Contribute to CORTX S3 Server

Please contribute to the [CORTX Open Source](https://github.com/Seagate/cortx/blob/main/CONTRIBUTING.md) initiative and join our movement to make data storage better, efficient, and more accessible.

### Reach Out to Us

Please refer to the [Support](SUPPORT.md) section for reaching out to us.
