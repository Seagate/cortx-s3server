package com.seagates3.response.generator;

import com.seagates3.response.ServerResponse;
import com.seagates3.response.formatter.xml.XMLResponseFormatter;

public class UserPolicyResponseGenerator {

	public ServerResponse generateAttachUserPolicyResponse() {
		return new XMLResponseFormatter().formatAttachUserPolicyResponse("AttachUserPolicy");
	}
	
	public ServerResponse generateDetachUserPolicyResponse() {
		return new XMLResponseFormatter().formatDetachUserPolicyResponse("DetachUserPolicy");
	}
}
