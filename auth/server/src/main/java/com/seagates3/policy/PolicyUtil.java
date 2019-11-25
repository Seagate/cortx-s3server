package com.seagates3.policy;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public
class PolicyUtil {

 public
  enum Services {
    S3,
    IAM
  }

  /**
   * Fetch the resource name from the resource ARN.
   *   ARN format -
   *         arn:aws:s3|iam:[region_name]:[account_id]:<resource_name>
   *   e.g. 1. arn:aws:s3:::SeagateBucket/TestObject
   *           returns 'SeagateBucket/TestObject'
   *        2. arn:aws:s3:::SeagateBucket/path/to/folder
   *           returns 'SeagateBucket/path/to/folder'
   * @param arn
   * @return - the bucket name
   */
  public static String
  getResourceFromResourceArn(String arn) {
    String tokens[] = arn.split(":");
    return tokens[5];
  }

  /**
   * Fetch the bucket name from ClientAbsoluteUri request parameter
   *   e.g. 1. /seagatebucket/dir-1/abc2
   *           returns 'seagatebucket'
   *        2. /testbucket/testobject
   *           returns 'testbucket'
   * @param uri - 'ClientAbsoluteUri' from request body
   * @return - the bucket name
   */
 public
  static String getBucketFromUri(String uri) {
    if (uri == null || uri.isEmpty()) {
      return null;
    }
    String[] arr = uri.split("/");
    if (arr[1] != null && !arr[1].isEmpty()) return arr[1];
    return null;
  }

  /**
      * Get all matching actions from Actions.json
      *
      * @param inputAction
      * @return
      */
 public
  static List<String> getAllMatchingActions(String inputAction) {
    List<String> matchingActions = new ArrayList<>();
    for (String action : Actions.getBucketOperations()) {
      if (isPatternForActionValid(action, inputAction)) {
        matchingActions.add(action);
      }
    }
    for (String action : Actions.getObjectOperations()) {
      if (isPatternForActionValid(action, inputAction)) {
        matchingActions.add(action);
      }
    }
    return matchingActions;
  }

  /**
 * Function that matches input string with given wildcard pattern
     *
     * @param input - Source string to match the pattern with
     * @param pattern - Pattern with wildchar characters to match. eg- GetObj*
 *or GetObjectAc?
     * @return - true/false
     */
 private
  static boolean isPatternForActionValid(String input, String pattern) {
    int inputLength = input.length();
    int patternLength = pattern.length();
    if (patternLength == 0) return (inputLength == 0);
    boolean[][] data = new boolean[inputLength + 1][patternLength + 1];
    for (int count = 0; count < inputLength + 1; count++)
      Arrays.fill(data[count], false);
    data[0][0] = true;
    // Only '*' can match with empty string
    for (int count = 1; count <= patternLength; count++)
      if (pattern.charAt(count - 1) == '*') data[0][count] = data[0][count - 1];
    for (int i = 1; i <= inputLength; i++) {
      for (int j = 1; j <= patternLength; j++) {
        if (pattern.charAt(j - 1) == '*')
          data[i][j] = data[i][j - 1] || data[i - 1][j];
        else if (pattern.charAt(j - 1) == '?' ||
                 input.charAt(i - 1) == pattern.charAt(j - 1))
          data[i][j] = data[i - 1][j - 1];
        else
          data[i][j] = false;
      }
    }
    return data[inputLength][patternLength];
  }

 public
  static boolean isActionValidForResource(List<String> actions,
                                          int slashPosition) {
    boolean isValid = true;
    for (String action : actions) {
      if ((Actions.getBucketOperations().contains(action)) &&
          slashPosition !=
              -1) {  // Bucket operation but object is present in resource
        isValid = false;
        break;
      } else if (Actions.getObjectOperations().contains(action) &&
                 slashPosition == -1) {  // object operation but object is
                                         // missing in resource
        isValid = false;
        break;
      }
    }
    return isValid;
  }

 public
  static List<String> convertCommaSeparatedStringToList(String str) {
    List<String> list = Arrays.asList(str.split("\\s*,\\s*"));
    return list;
  }
}
