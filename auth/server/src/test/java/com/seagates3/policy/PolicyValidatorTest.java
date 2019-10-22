package com.seagates3.policy;

import java.util.Map;
import java.util.TreeMap;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.powermock.core.classloader.annotations.PowerMockIgnore;
import org.powermock.core.classloader.annotations.PrepareForTest;
import org.powermock.modules.junit4.PowerMockRunner;

import com.seagates3.acl.ACLRequestValidator;
import com.seagates3.authorization.Authorizer;
import com.seagates3.response.ServerResponse;
import com.seagates3.response.generator.BucketPolicyResponseGenerator;
import com.seagates3.util.BinaryUtil;

import io.netty.handler.codec.http.HttpResponseStatus;

@RunWith(PowerMockRunner.class) @PowerMockIgnore({"javax.management.*"})
    @PrepareForTest(
        ACLRequestValidator.class) public class PolicyValidatorTest {

  String positiveJsonInput = null;
  String negativeJsonInput = null;
  Map<String, String> requestBody = null;

  @Before public void setup() {
    requestBody = new TreeMap<>();
  }

  /**
   * Test to validate json- Positive test with correct json
   */
  @Test public void validateBucketPolicy_Positive() {
    String positiveJsonInput =
        "{\r\n" + "  \"Id\": \"Policy1572590142744\",\r\n" +
        "  \"Version\": \"2012-10-17\",\r\n" + "  \"Statement\": [\r\n" +
        "    {\r\n" + "      \"Sid\": \"Stmt1572590138556\",\r\n" +
        "      \"Action\": [\r\n" + "        \"s3:GetBucketAcl\",\r\n" +
        "        \"s3:DeleteBucket\"\r\n" + "      ],\r\n" +
        "      \"Effect\": \"Allow\",\r\n" +
        "      \"Resource\": \"arn:aws:s3:::MyBucket\",\r\n" +
        "      \"Principal\": \"*\"\r\n" + "    }\r\n" + "  ]\r\n" +
        "}   \r\n" + "";
    requestBody.put("ClientAbsoluteUri", "/MyBucket");
    requestBody.put("Policy",
                    BinaryUtil.encodeToBase64String(positiveJsonInput));
    Authorizer authorizer = new Authorizer();
    ServerResponse response = authorizer.validatePolicy(requestBody);
    Assert.assertEquals(HttpResponseStatus.OK, response.getResponseStatus());
  }

  /**
   * Below will validate json with resource as NULL
   */
  @Test public void validateBucketPolicy_Resource_Null() {
    String negativeJsonInput =
        "{\r\n" + "  \"Id\": \"Policy1572590142744\",\r\n" +
        "  \"Version\": \"2012-10-17\",\r\n" + "  \"Statement\": [\r\n" +
        "    {\r\n" + "      \"Sid\": \"Stmt1572590138556\",\r\n" +
        "      \"Action\": [\r\n" + "        \"s3:GetBucketAcl\",\r\n" +
        "        \"s3:DeleteBucket\"\r\n" + "      ],\r\n" +
        "      \"Effect\": \"Allow\",\r\n" + "      \r\n" +
        "      \"Principal\": \"*\"\r\n" + "    }\r\n" + "  ]\r\n" +
        "}   \r\n" + "";
    requestBody.put("ClientAbsoluteUri", "/MyBucket");
    requestBody.put("Policy",
                    BinaryUtil.encodeToBase64String(negativeJsonInput));
    Authorizer authorizer = new Authorizer();
    ServerResponse response = authorizer.validatePolicy(requestBody);
    Assert.assertEquals(new BucketPolicyResponseGenerator()
                            .malformedPolicy("Missing required field Resource")
                            .getResponseBody(),
                        response.getResponseBody());
  }

  /**
   * Below will validate json with NULL Action value
   */
  @Test public void validateBucketPolicy_Action_Null() {
    String inputJson = "{\r\n" + "  \"Id\": \"Policy1572590142744\",\r\n" +
                       "  \"Version\": \"2012-10-17\",\r\n" +
                       "  \"Statement\": [\r\n" + "    {\r\n" +
                       "      \"Sid\": \"Stmt1572590138556\",\r\n" +
                       "      \r\n" + "      \"Effect\": \"Allow\",\r\n" +
                       "      \"Resource\": \"arn:aws:s3:::MyBucket\",\r\n" +
                       "      \"Principal\": \"*\"\r\n" + "    }\r\n" +
                       "  ]\r\n" + "}   \r\n" + "";

    requestBody.put("ClientAbsoluteUri", "/MyBucket");
    requestBody.put("Policy", BinaryUtil.encodeToBase64String(inputJson));
    Authorizer authorizer = new Authorizer();
    ServerResponse response = authorizer.validatePolicy(requestBody);
    Assert.assertEquals(new BucketPolicyResponseGenerator()
                            .malformedPolicy("Missing required field Action")
                            .getResponseBody(),
                        response.getResponseBody());
  }

  /**
   * Below will validate Bucket operation against resource with object
   */
  @Test public void
  validateBucketPolicy_Invalid_Action_Resource_Combination_Check1() {
    String inputJson =
        "{\r\n" + "  \"Id\": \"Policy1572590142744\",\r\n" +
        "  \"Version\": \"2012-10-17\",\r\n" + "  \"Statement\": [\r\n" +
        "    {\r\n" + "      \"Sid\": \"Stmt1572590138556\",\r\n" +
        "      \"Action\": [\r\n" + "        \"s3:GetBucketAcl\",\r\n" +
        "        \"s3:DeleteBucket\"\r\n" + "      ],\r\n" +
        "      \"Effect\": \"Allow\",\r\n" +
        "      \"Resource\": \"arn:aws:s3:::MyBucket/sample.txt\",\r\n" +
        "      \"Principal\": \"*\"\r\n" + "    }\r\n" + "  ]\r\n" +
        "}   \r\n" + "";
    requestBody.put("ClientAbsoluteUri", "/MyBucket");
    requestBody.put("Policy", BinaryUtil.encodeToBase64String(inputJson));
    Authorizer authorizer = new Authorizer();
    ServerResponse response = authorizer.validatePolicy(requestBody);
    Assert.assertEquals(
        new BucketPolicyResponseGenerator()
            .malformedPolicy(
                 "Action does not apply to any resource(s) in statement")
            .getResponseBody(),
        response.getResponseBody());
  }

  /**
   * Below will validate Object actions against bucket resource
   */
  @Test public void
  validateBucketPolicy_Invalid_Action_Resource_Combination_Check2() {
    String inputJson =
        "{\r\n" + "  \"Id\": \"Policy1572590142744\",\r\n" +
        "  \"Version\": \"2012-10-17\",\r\n" + "  \"Statement\": [\r\n" +
        "    {\r\n" + "      \"Sid\": \"Stmt1572590138556\",\r\n" +
        "      \"Action\": [\r\n" + "        \"s3:GetObjectAcl\",\r\n" +
        "        \"s3:GetObject\"\r\n" + "      ],\r\n" +
        "      \"Effect\": \"Allow\",\r\n" +
        "      \"Resource\": \"arn:aws:s3:::MyBucket\",\r\n" +
        "      \"Principal\": \"*\"\r\n" + "    }\r\n" + "  ]\r\n" +
        "}   \r\n" + "";
    requestBody.put("ClientAbsoluteUri", "/MyBucket");
    requestBody.put("Policy", BinaryUtil.encodeToBase64String(inputJson));
    Authorizer authorizer = new Authorizer();
    ServerResponse response = authorizer.validatePolicy(requestBody);
    Assert.assertEquals(
        new BucketPolicyResponseGenerator()
            .malformedPolicy(
                 "Action does not apply to any resource(s) in statement")
            .getResponseBody(),
        response.getResponseBody());
  }

  /**
   * Below will validate with wild card characters in actions
   */
  @Test public void
  validateBucketPolicy_ActionWithWildCardChars_Positive_Check() {
    String inputJson =
        "{\r\n" + "  \"Id\": \"Policy1572590142744\",\r\n" +
        "  \"Version\": \"2012-10-17\",\r\n" + "  \"Statement\": [\r\n" +
        "    {\r\n" + "      \"Sid\": \"Stmt1572590138556\",\r\n" +
        "      \"Action\": [\r\n" + "        \"s3:GetOb*\",\r\n" +
        "        \"s3:GetObject?cl\"\r\n" + "      ],\r\n" +
        "      \"Effect\": \"Allow\",\r\n" +
        "      \"Resource\": \"arn:aws:s3:::MyBucket/sample.txt\",\r\n" +
        "      \"Principal\": \"*\"\r\n" + "    }\r\n" + "  ]\r\n" +
        "}   \r\n" + "";
    requestBody.put("ClientAbsoluteUri", "/MyBucket");
    requestBody.put("Policy", BinaryUtil.encodeToBase64String(inputJson));
    Authorizer authorizer = new Authorizer();
    ServerResponse response = authorizer.validatePolicy(requestBody);
    Assert.assertEquals(HttpResponseStatus.OK, response.getResponseStatus());
  }

  /**
   * Below will validate Object actions against bucket resource along with wild
   * card characters in Actions
   */
  @Test public void
  validateBucketPolicy_ActionWithWildCardChars_Negative_Check() {
    String inputJson =
        "{\r\n" + "  \"Id\": \"Policy1572590142744\",\r\n" +
        "  \"Version\": \"2012-10-17\",\r\n" + "  \"Statement\": [\r\n" +
        "    {\r\n" + "      \"Sid\": \"Stmt1572590138556\",\r\n" +
        "      \"Action\": [\r\n" + "        \"s3:GetOb*\",\r\n" +
        "        \"s3:GetObject?cl\"\r\n" + "      ],\r\n" +
        "      \"Effect\": \"Allow\",\r\n" +
        "      \"Resource\": \"arn:aws:s3:::MyBucket\",\r\n" +
        "      \"Principal\": \"*\"\r\n" + "    }\r\n" + "  ]\r\n" +
        "}   \r\n" + "";
    requestBody.put("ClientAbsoluteUri", "/MyBucket");
    requestBody.put("Policy", BinaryUtil.encodeToBase64String(inputJson));
    Authorizer authorizer = new Authorizer();
    ServerResponse response = authorizer.validatePolicy(requestBody);
    Assert.assertEquals(
        new BucketPolicyResponseGenerator()
            .malformedPolicy(
                 "Action does not apply to any resource(s) in statement")
            .getResponseBody(),
        response.getResponseBody());
  }
}
