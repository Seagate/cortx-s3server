package com.seagates3.policy;

public
class IAMArnParser extends ArnParser {

public
  IAMArnParser() {
	 /**
	 * ARN format -> arn:partition:service:region:account:resource
	 * For iam service, region is always blank.
	 * For iam service, account is 12 digit number without hyphens or *.
	 */
    this.regEx = "arn:aws:iam::([0-9]{12}|[*]):[a-zA-Z0-9~@#$^*\\/_.-]+";
  }
}