package com.seagates3.exception;

import com.seagates3.response.ServerResponse;

public class GuardClauseException extends Exception {

	/**
	 * 
	 */
	private static final long serialVersionUID = -4647553014792334422L;
	
	private ServerResponse serverResponse;

	public GuardClauseException(ServerResponse serverResponse) {
		super();
		this.serverResponse = serverResponse;
	}

	public ServerResponse getServerResponse() {
		return serverResponse;
	}

}
