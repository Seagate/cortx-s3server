# Contribute to S3 Server 

- [1.0 Prerequisites](#10-Prerequisites)
- [1.2 Set up Git on your Development Box](#12-Set-up-Git-on-your-Development-Box)
- [1.3 Submit your changes](#13-Submit-your-changes)
   * [1.3.1 Clone the cortx-s3server repository](#131-Clone-the-cortx-s3server-repository)
   * [1.3.2 Code Commits](#132-Code-commits)
   * [1.3.3 Create a Pull Request](#133-Create-a-Pull-Request)
- [1.4 Run Jenkins and System Tests](#14-Run-Jenkins-and-System-Tests)
- [FAQs](FAQs)


Contributing to the S3 Server repository is a three-step process where you'll need to:

1. [Clone the cortx-s3server repository](#131-Clone-the-cortx-s3server-repository)
2. [Commit your Code](#132-Code-commits)
3. [Create a Pull Request](#133-Create-a-Pull-Request)

## 1.0 Prerequisites 

> Before you set up your GitHub, you'll need to  
>
> 1. Generate the SSH key on your development box using: 
>   ```
>     $ $ ssh-keygen -o -t rsa -b 4096 -C "seagate-email-address"
>  ```
> 2. Add the SSH key to your GitHub Account:
>
>    1. Copy the public key: `id_rsa.pub`. By default, your  public key is located at `/root/.ssh/id_rsa.pub`. 
>    2. Navigate to [GitHub SSH key settings](https://github.com/settings/keys) on your GitHub account.
>   
>          **Note:** Ensure that you've set your Seagate Email ID as the Primary Email Address associated with your GitHub Account. SSO will not work if you do not set your Seagate Email ID as your Primary Email Address.
>
>    3. Paste the SSH key you generated in Step 1 and select *Enable SSO*. 
>    4. Click **Authorize** to authorize SSO for your SSH key.  
> 3. [Create a Personal Access Token or PAT](https://help.github.com/en/github/authenticating-to-github/creating-a-personal-access-token).
>     - Ensure that you have enabled SSO for your PAT.

## 1.2 Setup Git on your Development Box

> **Before you begin:**
>
> 1. Update Git to the latest version. If you're on an older version of Git, you'll see errors in your commit hooks that look like this: 
> 
>     `$ git commit`
>     ```
>     git: 'interpret-trailers' is not a git command. 
>     See 'git --help'.
>     cannot insert change-id line in .git/COMMIT_EDITMSG
 >
 >  2. Install Fix for CentOS 7.x:
>
    >      `$ yum remove git`
    >    
    >      Download the [RPM file from here](https://packages.endpoint.com/rhel/7/os/x86_64/endpoint-repo-1.7-1.x86_64.rpm) and run command:
    >
    >      `$ yum -y install`
    > 
    >      `$ yum -y install git`
    >
  
Once you've installed the prerequisites, follow these steps to set up Git on your Development Box: 

1. Install git-clang-format using:
   
   `$ yum install git-clang-format`

2. Set up git config options using:

   ```
   $ git config --global user.name ‘Your Name’
   $ git config --global user.email ‘Your.Name@seagate.com’
   $ git config --global color.ui auto
   $ git config --global credential.helper cache 
   ```

## 1.3. Submit your changes

Before you can work on a GitHub feature, you'll need to clone the cortx-s3server repository.

### 1.3.1 Clone the cortx-s3server repository

Follow these steps to clone the repository to your gitHub account:

You'll need to *fork* the cortx-s3server repository to clone it into your private GitHub repository. 

1. Navigate to the Seagate 'cortx-s3server' repository homepage on GitHub. 
2. Click **Fork**.
3. Run the following commands in Git Bash:
 
   `$ git clone git@github.com:"your-github-id"/cortx-s3server.git`
 
4. Check out to the “main” branch using:

      `$ git checkout main`
     
      `$ git checkout -b 'your-local-branch-name`

### 1.3.2 Code Commits  

You can now make changes to the code and save them in your files.Use the command below to add files that need to be pushed to the git staging area:
         
- `$ git add foo/somefile.cc`

1. To commit your code changes use: 

   `$ git commit -m ‘comment’` 

   Enter your GID and an appropriate Feature or Change description in comments.

2. Check out your git log to view the details of your commit and verify the author name using:

   `$ git log` 

   **Note:** 
   If you need to change the author name for your commit, refer to the GitHub article on [Changing author info](https://docs.github.com/en/github/using-git/changing-author-info).

3. To Push your changes to GitHub, use:

   `$ git push origin 'your-local-branch-name'`

   Your output will look like the Sample Output below: 
   
   ```
   Sample Output
   ~~~~~~~~~~~~~~
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
   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   ```

### 1.3.3 Create a Pull Request 

1. Once you Push changes to GitHub, the output will display a URL for creating a Pull Request. (As shown in the sample code above.) 
2. You'll be redirected to GitHib remote. 
3. Select **main** from the Branches/Tags drop-down list.
4. Click **Create pull request** to create the pull request.
5. Add reviewers to your pull request to review and provide feedback on your changes.

## 1.4 Run Jenkins and System Tests

Creating a pull request automatically triggers Jenkins jobs and System tests. To familiarize yourself with jenkins, please visit the [Jenkins wiki page](https://en.wikipedia.org/wiki/Jenkins_(software)).

## FAQs

**Q.** How do I rebase my local branch to the latest main branch?

**A** Follow the steps mentioned below:

> $ git pull origin master
>
> $ git submodule update --init --recursive
>
> $ git checkout 'your-local-branch'
>
> $ git pull origin 'your-remote-branch-name'
>
> $ git submodule update --init --recursive
>
> $ git rebase origin/master

To resolve conflicts, follow the troubleshooting steps mentioned in git error messages. 

Reach out to our [SUPPORT](SUPPORT.md) team, if you have any questions or need further clarifications.

## All set & You're Awesome!

Let's make storage better, efficient, and accessible. Join us in our goal to reinvent a data-driven world, and contribute to CORTX Open Source initiative. 

CORTX Open Source team welcomes you! :relaxed:
