package com.seagates3.parameter.validator;

import java.util.Map;

public class UserPolicyParameterValidator extends AbstractParameterValidator {

	public Boolean isValidAttachParams(Map<String, String> requestBody) {
		return true;
	}
	
	public Boolean isValidDetachParams(Map<String, String> requestBody) {
		return true;
	}
}
