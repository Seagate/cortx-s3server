package com.seagates3.util;

public class ARNUtil {

    private static final String PARTITION = "seagate";
    private static final String SERVICE = "iam";

    public static String createARN(String accountId, String resource) {
        return String.format("arn:%s:%s::%s:%s", PARTITION, SERVICE,
                accountId, resource);
    }

    public static String createARN(String accountId, String resourceType,
            String resource) {
        return String.format("arn:%s:%s::%s:%s/%s", PARTITION, SERVICE,
                accountId, resourceType, resource);
    }
}
