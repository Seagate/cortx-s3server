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
   * @param inputAction
   * @return
   */
 public
  static List<String> getAllMatchingActions(String inputAction) {
    List<String> matchingActions = new ArrayList<>();
    for (String action : ActionsInitializer.getActionsMap().keySet()) {
      if (strmatch(action, inputAction)) {
        matchingActions.add(action);
      }
    }
    return matchingActions;
  }

  /**
   * Function that matches input str with given wildcard pattern
   * @param str
   * @param pattern
   * @param n
   * @param m
   * @return
   */
 private
  static boolean strmatch(String str, String pattern) {
    // empty pattern can only match with
    // empty string
    int inputStringLength = str.length();
    int patternLength = pattern.length();
    if (patternLength == 0) return (inputStringLength == 0);

    // lookup table for storing results of
    // subproblems
    boolean[][] lookup = new boolean[inputStringLength + 1][patternLength + 1];

    // initailze lookup table to false
    for (int i = 0; i < inputStringLength + 1; i++)
      Arrays.fill(lookup[i], false);

    // empty pattern can match with empty string
    lookup[0][0] = true;

    // Only '*' can match with empty string
    for (int j = 1; j <= patternLength; j++)
      if (pattern.charAt(j - 1) == '*') lookup[0][j] = lookup[0][j - 1];

    // fill the table in bottom-up fashion
    for (int i = 1; i <= inputStringLength; i++) {
      for (int j = 1; j <= patternLength; j++) {

        if (pattern.charAt(j - 1) == '*')
          lookup[i][j] = lookup[i][j - 1] || lookup[i - 1][j];

        // Current characters are considered as
        // matching in two cases
        // (a) current character of pattern is '?'
        // (b) characters actually match
        else if (pattern.charAt(j - 1) == '?' ||
                 str.charAt(i - 1) == pattern.charAt(j - 1))
          lookup[i][j] = lookup[i - 1][j - 1];
        else
          lookup[i][j] = false;
      }
    }
    return lookup[inputStringLength][patternLength];
  }

 public
  static boolean isActionValidForResource(List<String> actions,
                                          int slashPosition) {
    boolean isValid = true;
    for (String action : actions) {
      if ("1".equals(ActionsInitializer.getActionsMap().get(action)) &&
          slashPosition !=
              -1) {  // Bucket operation but object is present in resource
        isValid = false;
        break;
      } else if ("0".equals(ActionsInitializer.getActionsMap().get(action)) &&
                 slashPosition == -1) {  // object operation but object is
                                         // missing in resource
        isValid = false;
        break;
      }
    }
    return isValid;
  }
}
