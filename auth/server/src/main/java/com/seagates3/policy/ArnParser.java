
package com.seagates3.policy;

public
class ArnParser {

  // arn:aws:(s3|iam):[region]:[accountID]:<resource>
 protected
  String regEx = "arn:aws:(s3|iam):[A-Za-z0-9-]*:" +
                 "[a-zA-Z0-9~@#$^*\\\\/_.-]*:[a-zA-Z0-9~@#$^*\\\\/_.-]+";

  /**
   * validates ARN format for generic S3/IAM - services
   * arn:aws:(s3|iam):[region]:[accountID]:<resource>
   * @param arn
   * @return
   */
 public
  boolean isArnFormatValid(String arn) {
    boolean isArnValid = false;
    if (arn != null && arn.matches(regEx)) {
      isArnValid = true;
    }
    return isArnValid;
  }
}
