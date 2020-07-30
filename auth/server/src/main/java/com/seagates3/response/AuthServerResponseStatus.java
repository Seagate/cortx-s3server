package com.seagates3.response;

public enum AuthServerResponseStatus {
    OK(""),
    NO_SUCH_ENTITY("noSuchEntity"),
    INACTIVE_ACCESS_KEY("inactiveAccessKey"),
    INVALID_CLIENT_TOKEN_ID("invalidClientToken"),
    EXPIRED_CREDENTIAL("expiredCredential"),
    INCORRECT_SIGNATURE("incorrectSignature");

    private final String method;

    /**
     * @param text
     */
    private AuthServerResponseStatus(final String text) {
        this.method = text;
    }

    /* (non-Javadoc)
     * @see java.lang.Enum#toString()
     */
    @Override
    public String toString() {
        return method;
    }
}
