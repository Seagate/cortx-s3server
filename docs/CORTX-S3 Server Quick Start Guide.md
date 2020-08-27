# CORTX-S3 Server Quick Start Guide
This guide provides a step-by-step walkthrough for getting you CORTX-S3 Server-ready.

- [1.0 Prerequisites](#11-prerequisites)
- [1.1 Clone the CORTX-S3 Server Repository](#12-clone-the-cortx-s3-server-repository)
- [1.2 Installing dependencies](#13-installing-dependencies)
- [1.3 Code Compilation and Unit Test](#14-code-compilation-and-unit-test)
- [1.4 Test your Build using S3-CLI](#15-test-your-build-using-s3-cli)
- [1.5 Test a specific MOTR Version using CORX-S3 Server](#16-test-a-specific-motr-version-using-corx-s3-server)
- [1.6 Build S3 rpms](#16-Build-S3-rpms)

### 1.0 Prerequisites

<details>
<summary>Click to expand!</summary>
<p>

1. You'll need to set up SSC, Cloud VM, or a local VM on VMWare Fusion or Oracle VirtualBox. To know more, refer to the [LocalVMSetup](https://github.com/Seagate/cortx/blob/main/doc/LocalVMSetup.md) section.
2. Our CORTX Contributors will refer, clone, contribute, and commit changes via the GitHub server. You can access the latest code via [Github](https://github.com/Seagate/cortx). 
3. You'll need a valid GitHub Account. 
4. Before you clone your Git repository, you'll need to create the following:
    1. Follow the link to generate the [SSH Public Key](https://git-scm.com/book/en/v2/Git-on-the-Server-Generating-Your-SSH-Public-Key).
    2. Add the newly created SSH Public Key to [Github](https://github.com/settings/keys) and [Enable SSO](https://docs.github.com/en/github/authenticating-to-github/authorizing-an-ssh-key-for-use-with-saml-single-sign-on).
    3. When you clone your Github repository, you'll be prompted to enter your GitHub Username and Password. Refer to the article to [Generate Personal Access Token or PAT](https://docs.github.com/en/github/authenticating-to-github/creating-a-personal-access-token). Once you generate your Personal Access Token, enable SSO. 
    4. Copy your newly generated [PAT](https://github.com/settings/tokens) and enter it when prompted.   

    :page_with_curl: **Note:** From this point onwards, you'll need to execute all steps logged in as a **Root User**.

5. We've assumed that `git` is preinstalled. If not then follow these steps to install [Git](https://git-scm.com/book/en/v2/Getting-Started-Installing-Git).
   * To check your Git Version, use the command: `$ git --version`.
   
    :page_with_curl:**Note: We recommended that you install Git Version 2.x.x.**
   
6. Ensure that you've installed the following packages on your VM instance: 

    * Python Version 3.0
      * To check whether Python is installed on your VM, use one of the following commands: `--version`  , `-V` , or `-VV`
      * To install Python version 3.0, use: `$ yum install -y python3`
    * pip version 3.0: 
      * To check if pip is installed, use: `$ pip --version` 
      * To install pip3 use: `$ yum install python-pip3`  
    * Ansible: `$ yum install -y ansible` 
    * Extra Packages for Enterprise Linux: 
        * To check if epel is installed, use: `$ yum repolist` 
            * If epel was installed, you'll see it in the output list.
            * You might also see exclamation mark in front of the repositories id. Refer to the [Redhat Knowledge Base](https://access.redhat.com/solutions/2267871)
        * `$ yum install -y epel-release`
    * Verify if kernel-devel-3.10.0-1062 version package is installed, use: `$ uname -r`
    
7. You'll need to disable selinux and firewall. Run the following commands:

     `$ systemctl stop firewalld` no output
    
     `$ systemctl disable firewalld` no output
    
     `$ sestatus` should see `SELinux status: disabled'
    
     `$ setenforce 0` should see `setenforce: SELinux is disabled'
    
     `$ sed 's/SELINUX=enforcing/SELINUX=disabled/' /etc/sysconfig/selinux` should see `SELINUX=disabled'
    
     `$ shutdown -r now` should reboot your system. 
    
    :page_with_curl: **Notes:**
    
    1. If you're using cloud VM, go to your cloud VM website and select the VM. 
    2. First stop the VM and then start it again to complete the reboot process
    3. To use command line to shutdown your VM, use: `$ shutdown -r now` 
    4. Restart your VM.
                           
   Once you power on your VM, you can verify if selinux and firewall are disabled by using: `$ getenforce` you'll get a 'disabled' status.

All done! You are now ready for fetching CORTX-S3 Server repository!  

</p>
</details>  

### 1.1 Clone the CORTX-S3 Server Repository

You'll need to clone the S3 Server Repository from the main branch. To clone the S3 Server Repository, follow these steps:

```shell
$ git clone --recursive git@github.com:Seagate/cortx-s3server.git -b main   
$ cd cortx-s3server
$ git submodule update --init --recursive && git status
``` 
    
### 1.2 Installing dependencies

<details>
<summary>Before you begin</summary>
<p>
 
At some point during the execution of the `init.sh` script, it will prompt for the following passwords. Enter them as mentioned below:
   * SSH password: `<Enter root password of VM>`
   * Enter new password for openldap rootDN:: `seagate`
   * Enter new password for openldap IAM admin:: `ldapadmin`

</p>
</details> 

Whenever you clone your repository or make changes to the dependent packages, you'll be need to initialize your package:

1. Run the command:

```shell

   $ cd ./scripts/env/dev
   $ ./init.sh -a
```

2. You'll be prompted to provide your GitHub token, enter the PAT token that you generated in Step 4.iv. from the [1.0 Prerequisites section](https://github.com/cortx-s3server/blob/dev/docs/CORTX-S3%20Server%20Quick%20Start%20Guide.md#11-prerequisites).
3. In some cases, the `$ ./init.sh` fails to run. 
4. If the above command fails, run: `$ ./upgrade-enablerepo.sh` and then run: `$ ./init.sh`.

Refer to the image below to view the output of a successful `$ ./init.sh` run, where the `failed` field value should be zero.

![Successful ./init.sh run](https://github.com/Seagate/cortx/blob/assets/images/init_script_output.PNG)

Please read our [FAQs](https://github.com/Seagate/cortx/blob/master/doc/Build-Installation-FAQ.md) for troubleshooting errors.

### 1.3 Code Compilation and Unit Test

<details>
<summary>Before you begin</summary>
<p>

- Run the following commands from the main source directory.
- Set up the host system before you test your build, using the command: `$ ./update-hosts.sh`

</p>
</details>

To perform Unit and System Tests:

* Run the script `$ ./jenkins-build.sh -h`
  
:page_with_curl: **Notes:** 

* The above script automatically builds the code, and runs the unit & system tests in your local system. 
* For more details, check help: `$ ./jenkins-build.sh --help`
    
The image below illustrates the output log of a system test that is successful.
  
![Successful System Test Log](https://raw.githubusercontent.com/Seagate/cortx/assets/images/jenkins_script_output.PNG?token=AQJGZB6SHID2AXELMYSDZMK7KDYLU)


### 1.4 Test your Build using S3-CLI

<details>
<summary>Before you begin</summary>
<p>
    
Before your test your build, ensure that you have installed and configured the following:

1. Make sure you have installed `easy_install`.
    * To check if you have `easy_install`, run the command: `$ easy_install --version`
    * To install `easy_install`, run the command: `$ yum install python-setuptools python-setuptools-devel`
2. Ensure you've installed `pip`.
    * To check if you have pip installed, run the command: `$ pip --version`
    * To install pip, run the command: `$ python --version`
3. If you don't have Python Version 2.6.5+, then install python using: `$ python3 --version`.    
    *  If you don't have Python Version 3.3, then install python3 using: `$ easy_install pip`
4. Ensure that CORTX-S3 Server and its dependent services are running.
    1. To start CORTX-S3 Server and its dependent services, run the command: `$ ./jenkins-build.sh --skip_build --skip_tests` 
    2. To view the `PID` of the active S3 service, run the command: `$ pgrep s3` 
    3. To view the `PID` of the active Motr service, run the command: `$ pgrep m0`
5. Install the aws client and plugin
    1. To install aws client, use: `$ pip install awscli`
    2. To install the aws plugin, use: `$ pip install awscli-plugin-endpoint`
    3. To generate the aws Access Key ID and aws Secret Key, run commands:
         1. To check for help messages, run the command: `$ s3iamcli -h`
         2. Run the following command to create a new User: `$ s3iamcli CreateAccount -n < Account Name > -e < Email Id >` 
              * Enter the following ldap credentials:
                  User Id : `sgiamadmin`
                  Password : `ldapadmin`
              * Running the above command lists details of the newly created user including the `aws Access Key ID` and the `aws Secret Key`. 
              * Copy and save the Access and Secret Keys for the new user. 

6. To Configure AWS run the following commands:
   Keep the Access and Secret Keys generated in Step 4.iv. from the [1.0 Prerequisites section](https://github.com/cortx-s3server/blob/dev/docs/CORTX-S3%20Server%20Quick%20Start%20Guide.md#11-prerequisites) handy. 
   1.  Run `$ aws configure` and enter the following details:
        * `AWS Access Key ID [None]: < ACCESS KEY >`
        * `AWS Secret Access Key [None]: < SECRET KEY >`
        * `Default region name [None]: US`
        * `Default output format [None]: text`
   2. Configure the aws plugin Endpoint using:
    
        ```shell
        
        $ aws configure set plugins.endpoint awscli_plugin_endpoint
        $ aws configure set s3.endpoint_url https://s3.seagate.com
        $ aws configure set s3api.endpoint_url https://s3.seagate.com
        ```
   3. Run the following command to view the contents of your aws config file: `$ cat ~/.aws/config`
       
      The output is as shown below:

      ```shell
       
      [default]
      output = text
      region = US
      s3 = 
      endpoint_url = http://s3.seagate.com
      s3api =
      endpoint_url = http://s3.seagate.com
      [plugins]
      endpoint = awscli_plugin_endpoint
      ```
          
    4. Ensure that your aws credential file contains your Access Key Id and Secret Key by using: `$ cat ~/.aws/credentials`
</p>
</details>

Run the following test cases to check if your aws S3 Server build is working properly.

1. To Make a Bucket:

    `$ aws s3 mb s3://seagatebucket` 
    
    You will get the following output: 
  
    `make_bucket: seagatebucket`

2. To List your newly created Bucket:

    `$ aws s3 ls`

3. To Copy your local file (test_data) to remote (PUT):
  
    `$ aws s3 cp test_data s3://seagatebucket/`

   This will create a test_data object in your bucket. You can use any file for to test this step. 
   
   If you want to create a test_data file, use the command:

    `$ touch filepath/test_data`

4. To Move your local file to remote (PUT):

    `$ aws s3 mv test_data s3://seagatebucket/` 
    
    This command moves your local file test_data to the bucket and create a test_data object. 

5. To List your moved object:

    `$ aws s3 ls s3://seagatebucket`
    
6. To Remove an object:

    `$ aws s3 rm s3://seagatebucket/test_data` 
    
    You'll not be able to view the object when you list objects.

7. To Remove Bucket:
    
    `$ aws s3 rb s3://seagatebuckettest`

### 1.5 Test a specific MOTR Version using CORX-S3 Server

Let's say there is a version change in the Motr repository, and you want to skip re-installing the S3 Server. You can do so by using specific Motr commits and test the same on your S3 Server.

<details>
<summary>Before you begin</summary>
<p>

1. You'll need to copy the commit-id of your Motr code. You can search for specific commit-ids using:

    `$ git log`

    While viewing the log, to find the next commit, type `/^commit`, then use `n` and `N` to move to the next or previous commit. To search for the previous commit, use `?^commit`.
2. You'll have to work out of the main directory of your S3 Server repository.
3. Run `$ cd third_party/motr`. 
4. Paste the commit-id shown below:
   
   `$ git checkout Id41cd2b41cb77f1d106651c267072f29f8c81d0f`
   
5. Update your submodules:

    `$ git submodule update --init --recursive`

6. Build Motr:

    `$ cd ..`
    
    `$ ./build_motr.sh` 
</p>
</details>

Run the jenkins script to make sure that build and test is passed:

`$ cd ..`

`$ ./jenkins-build.sh`

Your success log will look like the output in the image below:

![Successful test log](https://raw.githubusercontent.com/Seagate/cortx/assets/images/jenkins_script_output.PNG?token=AQJGZB62MLLTZRMAGHPYPPK7KDYA6)

### 1.6 Build S3 rpms

1. Obtain the short git revision that has to be built using:

    ```shell
    
    $ git rev-parse --short HEAD
    44a07d2
    ```
2. To build S3 rpm, use:

    `$ ./rpms/s3/buildrpm.sh -G 44a07d2`

    :page_with_curl:**Note:** `44a07d2` is generated in Step 1. 

3. To build S3 rpm without Motr rpm dependency, use:

    `$ ./rpms/s3/buildrpm.sh -a -G 44a07d2`

4. To build s3iamcli rpm, use:

    `$ ./rpms/s3iamcli/buildrpm.sh -G 44a07d2`

All the built rpms will be available at `~/rpmbuild/RPMS/x86_64/`. You can copy these rpms to release VM for testing.

## You're all set & you're awesome!

If you have any queries, feel free to reach out to our [SUPPORT](SUPPORT.md) team.

## Contribute to CORTX S3 Server

Contribute to the CORTX Open Source initiative and join our movement to make data storage better, efficient, and more accessible.

CORTX Community Welcomes You! :relaxed:
