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
 * Original creation date: 17-Sep-2015
 */

package com.seagates3.validator;

/**
 * Util class containing common validation rules for S3 APIs.
 */
public class S3ValidatorUtil {

    final static int MAX_ARN_LENGTH = 2048;
    final static int MAX_ACCESS_KEY_ID_LENGTH = 32;
    final static int MAX_ACCESS_KEY_USER_NAME_LENGTH = 128;
    final static int MAX_ASSUME_ROLE_POLICY_DOC_LENGTH = 2048;
    final static int MAX_ITEMS = 1000;
    final static int MAX_MARKER_LENGTH = 320;
    final static int MAX_NAME_LENGTH = 64;
    final static int MAX_PATH_LENGTH = 512;
    final static int MAX_SAML_METADATA_LENGTH = 10000000;
    final static int MAX_SAML_PROVIDER_NAME_LENGTH = 128;
    final static int MIN_ACCESS_KEY_ID_LENGTH = 16;
    final static int MIN_ARN_LENGTH = 20;
    final static int MIN_SAML_METADATA_LENGTH = 1000;

    /**
     * TODO
     * Fix ACCESS_KEY_ID_PATTERN. It should be "[\\w]+"
     */
    final static String ACCESS_KEY_ID_PATTERN = "[\\w-]+";
    final static String ASSUME_ROLE_POLICY_DOC_PATTERN = "[\\u0009\\u000A\\u000D\\u0020-\\u00FF]+";
    final static String MARKER_PATTERN = "[\\u0020-\\u00FF]+";
    final static String NAME_PATTERN = "[\\w+=,.@-]+";
    final static String PATH_PATTERN = "(\\u002F)|(\\u002F[\\u0021-\\u007F]+\\u002F)";
    final static String PATH_PREFIX_PATTERN = "\\u002F[\\u0021-\\u007F]*";
    final static String SAML_PROVIDER_NAME_PATTERN = "[\\w._-]+";

    /**
     * Validate the name (user name, role name etc).
     * Length of the name should be between 1 and 64 characters.
     * It should match the patten "[\\w+=,.@-]+".
     *
     * @param name name to be validated.
     * @return true if name is valid.
     */
    public static Boolean isValidName(String name) {
        if (name == null) {
            return false;
        }

        if (!name.matches(NAME_PATTERN)) {
            return false;
        }

        return !(name.length() < 1 || name.length() > MAX_NAME_LENGTH);
    }

    /**
     * Validate the path.
     * Length of the name should be between 1 and 512 characters.
     * It should match the patten "(\\u002F)|(\\u002F[\\u0021-\\u007F]+\\u002F)".
     *
     * @param path path to be validated.
     * @return true if path is valid.
     */
    public static Boolean isValidPath(String path) {
        if (!path.matches(PATH_PATTERN)) {
            return false;
        }

        return !(path.length() < 1 || path.length() > MAX_PATH_LENGTH);
    }

    /**
     * Validate the max items requested while listing resources.
     * Allowed range is between 1 and 1000.
     *
     * @param maxItems max items requested in a page.
     * @return true if maxItems is valid.
     */
    public static Boolean isValidMaxItems(String maxItems) {
        int maxItemsInt;
        try {
            maxItemsInt = Integer.parseInt(maxItems);
        } catch (NumberFormatException | AssertionError ex) {
            return false;
        }

        return !(maxItemsInt < 1 || maxItemsInt > MAX_ITEMS);
    }

    /**
     * Validate the path prefix.
     * Length of the name should be between 1 and 512 characters.
     * It should match the patten "\\u002F[\\u0021-\\u007F]*".
     *
     * @param pathPrefix path to be validated.
     * @return true if pathPrefix is valid.
     */
    public static Boolean isValidPathPrefix(String pathPrefix) {
        if (!pathPrefix.matches(PATH_PREFIX_PATTERN)) {
            return false;
        }

        return !(pathPrefix.length() < 1 || pathPrefix.length() > MAX_PATH_LENGTH);
    }

    /**
     * Validate the marker.
     * Length of the marker should be between 1 and 320 characters.
     * It should match the patten "[\\u0020-\\u00FF]+".
     *
     * @param marker marker to be validated.
     * @return true if the marker is valid.
     */
    public static Boolean isValidMarker(String marker) {
        if (!marker.matches(MARKER_PATTERN)) {
            return false;
        }

        return !(marker.length() < 1 || marker.length() > MAX_MARKER_LENGTH);
    }

    /**
     * Validate the access key user name (user name, role name etc).
     * Length of the name should be between 1 and 128 characters.
     * It should match the patten "[\\w+=,.@-]+".
     *
     * @param name name to be validated.
     * @return true if name is valid.
     */
    public static Boolean isValidAccessKeyUserName(String name) {
        if (name == null) {
            return false;
        }

        if (!name.matches(NAME_PATTERN)) {
            return false;
        }

        return !(name.length() < 1 || name.length() > MAX_ACCESS_KEY_USER_NAME_LENGTH);
    }

    /**
     * Validate the access key id.
     * Length of the name should be between 16 and 32 characters.
     * It should match the patten "[\\w]+".
     *
     * @param accessKeyId access key id to be validated.
     * @return true if access key id is valid.
     */
    public static Boolean isValidAccessKeyId(String accessKeyId) {
        if (accessKeyId == null) {
            return false;
        }

        if (!accessKeyId.matches(ACCESS_KEY_ID_PATTERN)) {
            return false;
        }

        return !(accessKeyId.length() < MIN_ACCESS_KEY_ID_LENGTH
                || accessKeyId.length() > MAX_ACCESS_KEY_ID_LENGTH);
    }

    /**
     * Validate the access key status.
     * Accepted values for status are 'Active' or 'Inactive'
     *
     * @param status access key status to be validated.
     * @return true if access key id is valid.
     */
    public static Boolean isValidAccessKeyStatus(String status) {
        if (status == null) {
            return false;
        }

        return (status.equals("Active") || status.equals("Inactive"));
    }

    /**
     * Validate assume role policy document.
     * Length of the document should be between 1 and 2048 characters.
     * It should match the patten "[\\u0009\\u000A\\u000D\\u0020-\\u00FF]+".
     *
     * @param policyDoc access key id to be validated.
     * @return true if access key id is valid.
     */
    public static Boolean isValidAssumeRolePolicyDocument(String policyDoc) {
        if (policyDoc == null) {
            return false;
        }

        if (!policyDoc.matches(ASSUME_ROLE_POLICY_DOC_PATTERN)) {
            return false;
        }

        return !(policyDoc.length() < 1 || policyDoc.length() > MAX_ASSUME_ROLE_POLICY_DOC_LENGTH);
    }

    /**
     * Validate the SAMl provider name (user name, role name etc).
     * Length of the name should be between 1 and 128 characters.
     * It should match the patten "[\\w._-]+".
     *
     * @param name name to be validated.
     * @return true if name is valid.
     */
    public static Boolean isValidSamlProviderName(String name) {
        if (name == null) {
            return false;
        }

        if (!name.matches(SAML_PROVIDER_NAME_PATTERN)) {
            return false;
        }

        return !(name.length() < 1 || name.length() > MAX_SAML_PROVIDER_NAME_LENGTH);
    }

    /**
     * Validate the SAMl metadata (user name, role name etc).
     * Length of the metadata should be between 1000 and 10000000 characters.
     *
     * @param samlMetadataDocument SAML metadata.
     * @return true if name is valid.
     */
    public static Boolean isValidSAMLMetadata(String samlMetadataDocument) {
        if (samlMetadataDocument == null) {
            return false;
        }

        return !(samlMetadataDocument.length() < MIN_SAML_METADATA_LENGTH
                || samlMetadataDocument.length() > MAX_SAML_METADATA_LENGTH);
    }

    /**
     * Validate the ARN (role ARN, principal ARN etc).
     * Length of the name should be between 20 and 2048 characters.
     *
     * @param arn ARN.
     * @return true if name is valid.
     */
    public static Boolean isValidARN(String arn) {
        if (arn == null) {
            return false;
        }

        return !(arn.length() < MIN_ARN_LENGTH || arn.length() > MAX_ARN_LENGTH);
    }
}
