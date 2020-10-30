# S3 API

## Supported APIs

:page_with_curl: **Note:** All operations listed in the tables below, are S3 compliant.

**Table 1: Basic Operations**

| Service Operations | Bucket Operations | Operation on Objects | 
|:---|:---|:---|
| Get | Get Bucket (List Objects) | Get Object | 
| | Put Bucket | Put Object | 
| | Delete Bucket | Delete Object |
| | Head Bucket | Head Object |

**Table 2: Moderately Complex Operations**

:page_with_curl: **Note:** Operations have limited support for groups.

| Bucket Operations | Operation on Objects | 
|:---|:---|
| Get Bucket ACL | Get Object ACL |
| Put Bucket ACL | Put Object ACL |
| List Multipart Uploads | Delete Multiple Objects |

**Table 2.1: Additional Features** 

| Additional Features | 
|:---|
| Initiate Multipart Upload |
| Upload Part |
| Complete Multipart Upload |
| Abort Multipart Upload |
| List Part |

**Table 3: Advance Complex Operations** 

| Bucket Operations | 
|:---|
| Get Bucket Location |
| Get Bucket Tagging |
| Put Bucket Tagging |
| Delete Bucket Tagging |
| Get Bucket Policy </br> (Limited Condition Support) |
| Put Bucket Policy |
| Delete Bucket Policy |


## Supported Service API

- List all your buckets  
    `aws s3 ls`

## Supported Operations/APIs on Bucket

1.  GetBucket (List objects in the specified bucket)  
    `aws s3 ls s3://<your_bucket>`  
    `aws s3api list-objects --bucket <your_bucket>`  
    `aws s3api list-objects-v2 --bucket <your_bucket>`  

2.  Put bucket  
    `aws s3 mb s3://<your_bucket>`  
    `aws s3api create-bucket --bucket <your_bucket>`  

3.  Head bucket  
    `aws s3api head-bucket --bucket <your_bucket>`  

4.  Delete bucket  
    `aws s3 rb s3://<your_bucket>`  
    `aws s3api delete-bucket --bucket <your_bucket>`  

5.  Put Bucket ACL  
    `aws s3api put-bucket-acl --bucket <your_bucket>`  

6.  Get Bucket ACL  
    `aws s3api get-bucket-acl --bucket <your_bucket>`  

7.  Multipart uploads

    7.1    Create multipart upload in bucket  
           `aws s3api create-multipart-upload --bucket <your_bucket> --key <key>`  

    7.2    Upload part  
           `aws s3api upload-part --bucket <your_bucket> --key <key> --part-number 1 --body <part>`  

    7.3    Lists parts of a multipart upload  
           `aws s3api list-parts --bucket <your_bucket> --key <key> --upload-id <id>`  

    7.4    Abort/Complete multipart  
           `aws s3api complete-multipart-upload --multipart-upload <mpustruct_file> --bucket <your_bucket> --key <key> --upload-id <id>`  
           `aws s3api abort-multipart-upload --bucket <your_bucket> --key <key> --upload-id <id>`  

    7.5    Lists in-progress multipart uploads in a bucket  
           `aws s3api list-multipart-uploads --bucket <your_bucket>`  

8.  Put Bucket policy  
    `aws s3api put-bucket-policy --bucket <your_bucket> --policy <policy_json_file>`  

9.  Get Bucket policy  
    `aws s3api get-bucket-policy --bucket <your_bucket>`  

10. Delete bucket policy  
    `aws s3api delete-bucket-policy --bucket <your_bucket>`  

11. Put Bucket tagging  
    `aws s3api put-bucket-tagging --bucket <your_bucket> --tagging <tagging_json_file>`  

12. Get Bucket tagging  
    `aws s3api get-bucket-tagging --bucket <your_bucket>`  

13. Delete bucket tagging  
    `aws s3api delete-bucket-tagging --bucket <your_bucket>`  

14. Get bucket location  
    `aws s3api get-bucket-location --bucket <your_bucket>`  

## Supported Operations/APIs on Object

1.  Put Object  
    `aws s3api put-object --bucket <your_bucket> --key <key> --body <file>`  
    `aws s3 cp <local_file> s3://<your_bucket>/<key>`  

2.  Get Object  
    `aws s3api get-object --bucket <your_bucket> --key <key> <save_to_file>`  
    `aws s3 cp s3://<your_bucket>/<key> <local_file>`  

3.  Delete Object  
    `aws s3api delete-object --bucket <your_bucket> --key <key>`  

4.  Head Object  
    `aws s3api head-object --bucket <your_bucket> --key <key>`  

5.  Put Object ACL  
    `aws s3api put-object-acl --bucket <your_bucket> --key <key> --grant-* emailaddress=<email>`  

6.  Get Object ACL  
    `aws s3api get-object-acl --bucket <your_bucket> --key <key>`  

7.  Delete multiple objects  
    `aws s3api delete-objects --bucket <your_bucket> --delete <del_struct_json_file>`  

8.  Put Object tagging  
    `aws s3api put-object-tagging --bucket <your_bucket> --key <key> --tagging <tagging_json_struct>`  

9.  Get Object tagging  
    `aws s3api get-object-tagging --bucket <your_bucket> --key <key>`  

10. Delete Object tagging  
    `aws s3api delete-object-tagging --bucket <your_bucket> --key <key>`  

## S3 IAM APIs (using client tool - s3iamcli)

1.  Create an account  
    `s3iamcli CreateAccount -n <Account Name> -e <Email Id>`  

2.  Delete an account  
    `s3iamcli DeleteAccount -n <Account Name>`  

3.  List accounts  
    `s3iamcli ListAccounts`  

4.  Create IAM User, with optional path  
    `s3iamcli CreateUser -n <User Name> [-p path]`  

5.  Change/update the name of existing IAM user  
    `s3iamcli UpdateUser -n <Old User Name> --new_user <New User Name> [-p <New Path>]`  

6.  Delete IAM user  
    `s3iamcli DeleteUser -n <User Name>`  

7.  Create access key for IAM user  
    `s3iamcli CreateAccessKey -n <User Name>`  

8.  List access keys/secret keys of IAM user  
    `s3iamcli ListAccessKeys -n <User Name>`  

9.  Delete access key of IAM user  
    `s3iamcli DeleteAccesskey -k <Access Key Id> -n <User Name>`  

10. Update the status of access key of IAM user (Change status to active or inactive)  
    `s3iamcli UpdateAccessKey -k <Access Key Id> -s <Active/Inactive> -n <User Name>`  

11. List all IAM users of current account, matching the path prefix  
    `s3iamcli ListUsers [-p <Path Prefix>]`  

12. Change the password of an IAM user  
    `s3iamcli ChangePassword --old_password <Old User Password> --new_password <New User Password>`  

13. Create temporary authentication credential for an account or IAM user  
    `s3iamcli GetTempAuthCredentials -a <Account Name> --password <Account Password> [-d <Duration in seconds>] [-n <User Name>]`  

14. Creates a password for the specified account  
    `s3iamcli CreateAccountLoginProfile -n <Account Name> --password <Account Password> [--password-reset-required |--no-password-reset-required]`  

15. Updates/changes password for the specified account  
    `UpdateAccountLoginProfile -n <Account Name> [--password <Account Password>] [--password-reset-required|--no-password-reset-required]`  

16. Retrieves the account name and password-creation date for the specified account  
    `s3iamcli GetAccountLoginProfile -n <Account Name>`  

17. Creates a password for the specified IAM user  
    `s3iamcli CreateUserLoginProfile -n <User Name> --password <User Password> [--password-reset-required |--no-password-reset-required]`  

18. Updates/changes password for the specified IAM user  
    `s3iamcli UpdateUserLoginProfile -n <User Name> [--password <User Password>] [--password-reset-required |   --no-password-reset-required]`  

19. Retrieves the user name and password-creation date for the specified IAM user  
    `s3iamcli GetUserLoginProfile -n <User Name>`  
