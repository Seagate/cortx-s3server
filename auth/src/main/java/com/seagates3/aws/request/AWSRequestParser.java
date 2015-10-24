/*
 * COPYRIGHT 2015 SEAGATE LLC
 *
 * THIS DRAWING/DOCUMENT, ITS SPECIFICATIONS, AND THE DATA CONTAINED
 * HEREIN, ARE THE EXCLUSIVE PROPERTY OF SEAGATE TECHNOLOGY
 * LIMITED, ISSUED IN STRICT CONFIDENCE AND SHALL NOT, WITHOUT
 * THE PRIOR WRITTEN PERMISSION OF SEAGATE TECHNOLOGY LIMITED,
 * BE REPRODUCED, COPIED, OR DISCLOSED TO A THIRD PARTY, OR
 * USED FOR ANY PURPOSE WHATSOEVER, OR STORED IN A RETRIEVAL SYSTEM
 * EXCEPT AS ALLOWED BY THE TERMS OF SEAGATE LICENSES AND AGREEMENTS.
 *
 * YOU SHOULD HAVE RECEIVED A COPY OF SEAGATE'S LICENSE ALONG WITH
 * THIS RELEASE. IF NOT PLEASE CONTACT A SEAGATE REPRESENTATIVE
 * http://www.seagate.com/contact
 *
 * Original author:  Arjun Hariharan <arjun.hariharan@seagate.com>
 * Original creation date: 22-Oct-2015
 */
package com.seagates3.aws.request;

import com.seagates3.authserver.AuthServerConfig;
import java.util.Map;

import io.netty.handler.codec.http.HttpRequest;

import com.seagates3.model.ClientRequestToken;
import io.netty.handler.codec.http.HttpHeaders;
import java.io.UnsupportedEncodingException;
import java.net.URLEncoder;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Iterator;
import java.util.List;
import java.util.TreeMap;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public abstract class AWSRequestParser {

    /*
     * Parse the client request.
     */
    public abstract ClientRequestToken parse(HttpRequest httpRequest);

    public abstract ClientRequestToken parse(Map<String, String> requestBody);

    /*
     * Parse the authorization header.
     */
    public abstract void authHeaderParser(String authorizationHeaderValue,
            ClientRequestToken clientRequestToken);

    /*
     * Parse the request headers.
     */
    public void parseRequestHeaders(HttpRequest httpRequest,
            ClientRequestToken clientRequestToken) {
        HttpHeaders httpHeaders = httpRequest.headers();

        clientRequestToken.setSessionToken(httpHeaders.get("x-amz-security-token"));
        clientRequestToken.setHttpMethod(httpRequest.getMethod().toString());

        String []tokens = httpRequest.getUri().split("\\?", -1);
        clientRequestToken.setUri(tokens[0]);

        String canonicalUri = convertToCanonicalUri(httpHeaders.get("host"), tokens[0]);
        clientRequestToken.setCanonicalUri(canonicalUri);

        String canonicalQuery;
        if(tokens.length == 2) {
            canonicalQuery = formatCanonicalQuery(tokens[1]);
        } else {
            canonicalQuery = "";
        }

        clientRequestToken.setCanonicalQuery(canonicalQuery);
        clientRequestToken.setHashedPayload(httpHeaders.get("x-amz-content-sha256"));
        clientRequestToken.setRequestDate(httpHeaders.get("x-amz-date"));
    }

    /*
     * Parse the client request.
     */
    public void parseRequestHeaders(Map<String, String> requestBody,
            ClientRequestToken clientRequestToken) {

        clientRequestToken.setSessionToken(requestBody.get("x-amz-security-token"));
        clientRequestToken.setHttpMethod(requestBody.get("Method"));

        clientRequestToken.setUri(requestBody.get("ClientAbsoluteUri"));

        String canonicalUri = convertToCanonicalUri(requestBody.get("host"),
                requestBody.get("ClientAbsoluteUri"));
        clientRequestToken.setCanonicalUri(canonicalUri);

        String canonicalQuery;
        canonicalQuery = formatCanonicalQuery(requestBody.get("ClientQueryParams"));

        clientRequestToken.setCanonicalQuery(canonicalQuery);
        clientRequestToken.setHashedPayload(requestBody.get("x-amz-content-sha256"));
        clientRequestToken.setRequestDate(requestBody.get("x-amz-date"));
    }

    /*
     * For S3 APIs
     *   If the host is path style, then remove the bucket name from the URI
     *   to form canonical URI.
     *
     * Ex - host = s3.seagate.com
     *      URI = /bucket/myfile
     *      CanonicalURI = /myfile
     */
    private String convertToCanonicalUri(String host, String uri) {
        List<String> s3Endpoints = Arrays.asList(AuthServerConfig.getEndpoints());
        List<String> uriEndpoints = new ArrayList<>();
        uriEndpoints.add(AuthServerConfig.getDefaultEndpoint());
        uriEndpoints.addAll(s3Endpoints);

        String patternToMatch;
        Pattern pattern;
        Matcher matcher;

        /*
         * Iterate over the endpoints to check if the host is an s3 end point.
         * If the host is an s3 end point and is in path style, then remove the
         * bucket name from the URI and return it.
         */
        for(String endPoint : uriEndpoints) {
            patternToMatch = String.format("(^[\\w]*).%s[:\\d]*$", endPoint);
            pattern = Pattern.compile(patternToMatch);
            matcher = pattern.matcher(host);

            if (matcher.matches())
            {
                /*
                 * Host name is in virtual host format.
                 * Return the uri as it is.
                 */
                return uri;
            }

            patternToMatch = String.format("%s[:\\d]*$", endPoint);
            pattern = Pattern.compile(patternToMatch);
            matcher = pattern.matcher(host);
            if(matcher.matches()) {
                /*
                 * It is an s3 end point and the host is in path style format.
                 * Remove the bucket name from the URI and return it.
                 */
                pattern = Pattern.compile("/.*/.*");
                matcher = pattern.matcher(uri);

                /*
                 * Check if the uri in the following format
                 *    /[bucket]/../..
                 */
                if(matcher.matches()) {
                    String[] tokens = uri.split("/[\\w]*/",2);
                    String canonicalUri = "/" + tokens[1];
                    return canonicalUri;
                } else {
                    /*
                     * Unrecognized pattern. Return URI as it is.
                     */
                    return uri;
                }
            }
        }

        /*
         * The host is not an s3 end point. It could be an IAM or STS end point.
         * Return the URI as it is.
         */
        return uri;
    }

    /*
     * To construct the canonical query string, complete the following steps:
     *
     * 1. URI-encode each parameter name and value according to the following rules:
     *   a. Do not URL-encode any of the unreserved characters that RFC 3986 defines:
     *      A-Z, a-z, 0-9, hyphen ( - ), underscore ( _ ), period ( . ), and tilde ( ~ ).
     *   b. Percent-encode all other characters with %XY, where X and Y are
     *      hexadecimal characters (0-9 and uppercase A-F).
     *      For example, the space character must be encoded as %20
     *      (not using '+', as some encoding schemes do) and extended UTF-8
     *      characters must be in the form %XY%ZA%BC.
     *
     * 2. Sort the encoded parameter names by character code
     *     (that is, in strict ASCII order). For example, a parameter name
     *     that begins with the uppercase letter F (ASCII code 70) precedes a
     *     parameter name that begins with a lowercase letter b (ASCII code 98).
     *
     * 3. Build the canonical query string by starting with the first
     *    parameter name in the sorted list.
     *
     * 4. For each parameter, append the URI-encoded parameter name, followed
     *    by the character '=' (ASCII code 61), followed by the URI-encoded
     *    parameter value.
     *    Use an empty string for parameters that have no value.
     *
     * 5. Append the character '&' (ASCII code 38) after each parameter value
     *    except for the last value in the list.
     */
    private String formatCanonicalQuery(String query) {
        if(query.isEmpty()) {
            return "";
        }

        Map<String, String> queryParams = new TreeMap<>();
        String[] tokens =  query.split("&");
        for(String token: tokens) {
            String[] subTokens = token.split("=");
            if(subTokens.length == 2) {
                queryParams.put(subTokens[0], subTokens[1]);
            } else {
                queryParams.put(subTokens[0], "");
            }
        }

        String canonicalString = "";
        Iterator<Map.Entry<String, String>> entries = queryParams.entrySet().iterator();
        while (entries.hasNext()) {
            Map.Entry<String, String> entry = entries.next();
            try {
                canonicalString += URLEncoder.encode(entry.getKey(), "UTF-8") + "=";
                canonicalString += URLEncoder.encode(entry.getValue(), "UTF-8");
            } catch (UnsupportedEncodingException ex) {

            }

            if(entries.hasNext()) {
                canonicalString += "&";
            }
        }

        return canonicalString;
    }
}
