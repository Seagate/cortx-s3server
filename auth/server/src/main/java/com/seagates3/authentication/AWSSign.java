package com.seagates3.authentication;

import java.util.Collections;
import java.util.HashMap;
import java.util.Map;

import com.seagates3.exception.InvalidTokenException;
import com.seagates3.model.Requestor;

public interface AWSSign {
    /**
     * Map the AWS signing algorithm against the corresponding hashing function.
     */
    public static final Map<String, String> AWSHashFunction =
            Collections.unmodifiableMap(
                    new HashMap<String, String>() {
                        {
                            put("AWS4-HMAC-SHA256", "hashSHA256");
                        }
                    });

    /*
     * Authenticate the request using AWS algorithm.
     */
    public Boolean authenticate(ClientRequestToken clientRequestToken,
                       Requestor requestor) throws InvalidTokenException;
}
