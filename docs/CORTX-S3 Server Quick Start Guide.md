# CORTX-S3 Server Quick Start Guide
This guide provides a step-by-step walkthrough for getting you CORTX-S3 Server-ready.

- [1.1 Prerequisites](#11-prerequisites)
  * [1.2 Clone the CORTX-S3 Server Repository](#12-clone-the-cortx-s3-server-repository)
    + [1.2.1 Create a local repository](#121-create-a-local-repository)
- [1.3 Installing dependencies](#13-installing-dependencies)
- [1.4 Code Compilation and Unit Test](#14-code-compilation-and-unit-test)
    + [1.4.1 Perform Unit and System Tests](#141-perform-unit-and-system-tests)
- [1.5 Test your Build using S3-CLI](#15-test-your-build-using-s3-cli)
    + [1.5.1 Test Cases](#151-test-cases)
- [1.6 Test a specific MOTR Version using CORX-S3 Server](#16-test-a-specific-motr-version-using-corx-s3-server)

## 1.1 Prerequisites

<details>
<summary>Click to expand!</summary>
<p>

1. You'll need to set up SSC, Cloud VM, or a local VM on VMWare Fusion or Oracle VirtualBox. To know more, refer to the [LocalVMSetup](LocalVMSetup.md) section.
2. Our CORTX Contributors will refer, clone, contribute, and commit changes via the GitHub server. You can access the latest code via [Github](https://github.com/Seagate/cortx). 
3. You'll need a valid GitHub Account. 
4. Before you clone your Git repository, you'll need to create the following:
    1. Follow the link to generate the [SSH Public Key](https://git-scm.com/book/en/v2/Git-on-the-Server-Generating-Your-SSH-Public-Key).
    2. Add the newly created SSH Public Key to [Github](https://github.com/settings/keys) and [Enable SSO](https://docs.github.com/en/github/authenticating-to-github/authorizing-an-ssh-key-for-use-with-saml-single-sign-on).
    3. When you clone your Github repository, you'll be prompted to enter your GitHub Username and Password. Refer to the article to [Generate Personal Access Token or PAT](https://github.com/settings/tokens). Once you generate your Personal Access Token, enable SSO. 
    4. Copy your newly generated [PAT](https://github.com/settings/tokens) and enter it when prompted.   

:page_with_curl: **Note:** From this point onwards, you'll need to execute all steps logged in as a **Root User**.

5. We've assumed that `git` is preinstalled. If not then follow these steps to install [Git](https://github.com/Seagate/cortx/blob/master/doc/ContributingToCortxS3.md).
   * To check your Git Version, use the command: `$ git --version`.
   
    :page_with_curl:**Note: We recommended that you install Git Version 2.x.x.**
   
6. Ensure that you've installed the following packages on your VM instance: 
    * Python Version 3.0
      * To check whether Python is installed on your VM, use one of the following commands: `--version`  , `-V` , or `-VV`
      * To install Python version 3.0, use: `$ yum install -y python3`
    * pip version 3.0: 
      * To verify your pip version use: `pip --version` 
      * To install pip3 use: `yum install python-pip3`  
    * Ansible: `$ yum install -y ansible` 
    * Extra Packages for Enterprise Linux: `$ yum install -y epel-release`
    * Verify if kernel-devel-3.10.0-1062 version package is installed, use: `uname -r`
7. You'll need to disable selinux and firewall. Run the following commands:

    ```shell
    $ systemctl stop firewalld
    $ systemctl disable firewalld
    $ sestatus
    $ setenforce 0
    sed 's/SELINUX=enforcing/SELINUX=disabled/' /etc/sysconfig/selinux
    ```
   1. To shutdown your VM, use: `$ shutdown -r now` 
   2. You will need to restart your VM. Once you power on your VM, you can verify if selinux and firewall are disabled by using: 
      
      `$ getenforce` 
      
      You'll get a 'disabled' status.

All done! You are now ready for fetching CORTX-S3 Server repository!  

</p>
</details>  

## 1.2 Clone the CORTX-S3 Server Repository

You'll need to clone the S3 Server Repository from the main branch. To clone the S3 Server Repository, follow these steps:

<details>
<summary>Click to expand!</summary>
<p> 

```shell
$ git clone --recursive git@github.com:Seagate/cortx-s3server.git -b main   
$ cd cortx-s3server
$ git submodule update --init --recursive && git status
``` 
</p>
</details>  
    
## 1.3 Installing dependencies

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

<details>
<summary>Click to expand!</summary>
<p>
   
```shell
   $ cd ./scripts/env/dev
   $ ./init.sh -a
```
    
* In some cases, the `./init.sh` fails to run. 
* If the above command fails, run `./upgrade-enablerepo.sh` and then run `./init.sh`.
  
Refer to the image below to view the output of a successful `./init.sh` run.
The where `failed` field value should be zero.

<img src="../assets/images/init_script_output.PNG?raw=true"></p>

Please refer to our [FAQs](https://github.com/Seagate/cortx/blob/master/doc/Build-Installation-FAQ.md) for troubleshooting errors.



## 1.4 Code Compilation and Unit Test

**Before you begin**

You'll need to run the following commands from the main source directory.

### 1.4.1 Perform Unit and System Tests

> Before you test your build, you'll need to setup the host system using the command:
> `$ ./update-hosts.sh`

 1. Run the `jenkins-build.sh` script.
    * The above script automatically builds the code, and runs the unit & system tests in your local system. 
    * Check help for more details.  
2. If the `/usr/local/bin` does not exist, you'll need to add the path using:  

    `$PATH=$PATH:/usr/local/bin`
  
   * The image below illustrates the output log of a system test that is successful.
  
<p align="center"><img src="../../assets/images/jenkins_script_output.PNG?raw=true"></p>

## 1.5 Test your Build using S3-CLI

**Before you begin:**

Before your test your build, ensure that you have installed and configured the following:

1. Make sure you have installed `easy_install`.

    * To check if you have `easy_install`, run the command: 
   
      `$ easy_install --version`
    * To install `easy_install`, run the command: 
    
      `$ yum install python-setuptools python-setuptools-devel`
2. Ensure you've installed `pip`.
    
    * To check if you have pip installed, run the command: 
    
      `$ pip --version`
    
    * To install pip, run the command: 
    
      `$ python --version`

3. If you don't have Python Version 2.6.5+, then install python using: 

      `$ python3 --version`.    

    *  If you don't have Python Version 3.3, then install python3 using:

        `$ easy_install pip`

4. Ensure that CORTX-S3 Server and its dependent services are running.

    1. To start CORTX-S3 Server and its dependent services, run the command:
        
         `$ ./jenkins-build.sh --skip_build --skip_tests` 

    2. To view the `PID` of the active S3 service, run the command:
      
        `$ pgrep s3` 

    3. To view the `PID` of the active Motr service, run the command: 
    
        `$ pgrep m0`

5. Install the aws client and plugin

    1. To install aws client, use:
          
          `$ pip install awscli`

      2. To install the aws plugin, use:
      
          `$ pip install awscli-plugin-endpoint`
  
      3. To generate the aws Access Key ID and aws Secret Key, run commands:
        
         1. To check for help messages, run the command:
          
            `$ s3iamcli -h`
            
         2. Run the following command to create a new User:
          
              `$ s3iamcli CreateAccount -n < Account Name > -e < Email Id >` 
          
               *   Enter the following ldap credentials:
            
                  User Id : `sgiamadmin`
          
                  Password : `ldapadmin`
               
              > * Running the above command lists details of the newly created user including the `aws Access Key ID` and the `aws Secret Key`. 
              > * Copy and save the Access and Secret Keys for the new user. 
  
6. To Configure AWS run the following commands:

    **Before you begin:**
    
    You'll need to keep the Access and Secret Keys generated in Step - 3.2 handy. 

   1.  Run `$ aws configure` and enter the following details:

        * `AWS Access Key ID [None]: < ACCESS KEY >`

        * `AWS Secret Access Key [None]: < SECRET KEY >`

        * `Default region name [None]: US`

        * `Default output format [None]: text`

    2. Configure the aws plugin Endpoint using:

        * `$ aws configure set plugins.endpoint awscli_plugin_endpoint`

        * `$ aws configure set s3.endpoint_url https://s3.seagate.com`
        
        *  `$ aws configure set s3api.endpoint_url https://s3.seagate.com`
        
        * Run the following command to view the contents of your aws config file: 

          `$ cat ~/.aws/config`

        * The output is as shown below:

          ```
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
          
    3. Ensure that your aws credential file contains your Access Key Id and Secret Key by using: 

        `$ cat ~/.aws/credentials`

### 1.5.1 Test Cases

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

    `touch filepath/test_data`

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

## 1.6 Test a specific MOTR Version using CORX-S3 Server

Let's say there is a version change in the Motr repository, and you want to skip re-installing the S3 Server. You can do so by using specific Motr commits and test the same on your S3 Server.

You'll need to copy the commit-id of your Motr code. You can search for specific commit-ids using:

`git log`

While viewing the log, to find the next commit, type `/^commit`, then use `n` and `N` to move to the next or previous commit. To search for the previous commit, use `?^commit`.

**Before you begin**

You'll need to work out of the main directory of your S3 Server repository.


1. Run `$ cd third_party/motr`. 

2. Paste the commit-id shown below:
   
   `git checkout Id41cd2b41cb77f1d106651c267072f29f8c81d0f`
   
3. Update your submodules:

    `$ git submodule update --init --recursive`

4. Build Motr:

    `cd ..`
    
    `./build_motr.sh` 

6. Run the jenkins script to make sure that build and test is passed:

    `cd ..`

    `./jenkins-build.sh`

    Your success log will look like the output in the image below:

<p align="center"><img src="../../assets/images/jenkins_script_output.PNG?raw=true"></p>

## You're all set & you're awesome!

In case of any queries, feel free to reach out to our [SUPPORT](SUPPORT.md) team.

Contribute to Seagate's open-source initiative and join our movement to make data storage better, efficient, and more accessible.

Seagate CORTX Community Welcomes You! :relaxed:
