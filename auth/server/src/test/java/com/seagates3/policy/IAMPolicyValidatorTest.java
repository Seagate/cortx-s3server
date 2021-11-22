package com.seagates3.policy;

import static org.junit.Assert.fail;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.powermock.api.mockito.PowerMockito;
import org.powermock.core.classloader.annotations.PowerMockIgnore;
import org.powermock.core.classloader.annotations.PrepareForTest;
import org.powermock.modules.junit4.PowerMockRunner;

import com.seagates3.authserver.AuthServerConfig;
import com.seagates3.response.ServerResponse;

import io.netty.handler.codec.http.HttpResponseStatus;

@RunWith(PowerMockRunner.class)
@PrepareForTest({BucketPolicyValidator.class, AuthServerConfig.class})
@PowerMockIgnore({"javax.management.*"})
public class IAMPolicyValidatorTest {
	String positiveJsonInput = null;
	String negativeJsonInput = null;
	PolicyValidator validator = null;
	  
	@Before
	public void setUp() throws Exception {
		  positiveJsonInput = null;
		  negativeJsonInput = null;
		  validator = new IAMPolicyValidator();
		PowerMockito.mockStatic(AuthServerConfig.class);
	    PowerMockito.doReturn("2012-10-17")
	        .when(AuthServerConfig.class, "getPolicyVersion");
	    PowerMockito.doReturn("0000").when(AuthServerConfig.class, "getReqId");
	}
	

	@Test
	public void testValidatePolicy_Positive() {
		String positiveJsonInput =
				"{\r\n" + "\"Version\": \"2012-10-17\",\r\n" +
                "  \"Statement\": [\r\n" + "    {\r\n" +
                "      \"Sid\": \"Stmt1632740110513\",\r\n" +
                "      \"Action\": [\r\n" +
                "        \"s3:PutBucketAcljhghsghsd\"\r\n" +
                "      ],\r\n" + "      \"Effect\": \"Allow\",\r\n" +
                "      \"Resource\": \"arn:aws:iam::buck1\"\r\n" +
                "	  \r\n" + "    }\r\n" + "\r\n" + "  ]\r\n" +
                "}";
		    ServerResponse response = validator.validatePolicy(null,positiveJsonInput);
		    Assert.assertEquals(null, response);
	}
}
