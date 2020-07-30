package com.seagates3.util;

import static org.junit.Assert.*;

import org.junit.Test;

import io.netty.handler.codec.http.HttpMethod;

public class ACLPermissionUtilTest {

    /**
     * Test method for com.seagates3.util.ACLPermissionUtil#getACLPermission
     */
    @Test
    public void testGetACLPermission() {

        // List all buckets
      assertTrue(ACLPermissionUtil.getACLPermission(
                     HttpMethod.GET, "/", null) == ACLPermissionUtil.ACL_READ);

        // List bucket
      assertTrue(ACLPermissionUtil.getACLPermission(HttpMethod.GET, "/bkirunge",
                                                    null) ==
                 ACLPermissionUtil.ACL_READ);

        // Head bucket
      assertTrue(ACLPermissionUtil.getACLPermission(HttpMethod.HEAD,
                                                    "/bkirunge", null) ==
                 ACLPermissionUtil.ACL_READ);

        // Get Object in bucket
      assertTrue(ACLPermissionUtil.getACLPermission(
                     HttpMethod.GET, "/bkirunge/test.txt", null) ==
                 ACLPermissionUtil.ACL_READ);

        // Head Object in bucket
      assertTrue(ACLPermissionUtil.getACLPermission(
                     HttpMethod.HEAD, "/bkirunge/test.txt", null) ==
                 ACLPermissionUtil.ACL_READ);

        // Create bucket
      assertTrue(ACLPermissionUtil.getACLPermission(HttpMethod.PUT, "/bkirunge",
                                                    null) ==
                 ACLPermissionUtil.ACL_WRITE);

        // Upload object in bucket
      assertTrue(ACLPermissionUtil.getACLPermission(
                     HttpMethod.PUT, "/bkirunge/test.txt", null) ==
                 ACLPermissionUtil.ACL_WRITE);

        // POST object
      assertTrue(ACLPermissionUtil.getACLPermission(
                     HttpMethod.POST, "/bkirunge/test.txt", null) ==
                 ACLPermissionUtil.ACL_WRITE);

        // Delete bucket
      assertTrue(ACLPermissionUtil.getACLPermission(HttpMethod.DELETE,
                                                    "/bkirunge", null) ==
                 ACLPermissionUtil.ACL_WRITE);

        // Delete object in bucket
      assertTrue(ACLPermissionUtil.getACLPermission(
                     HttpMethod.DELETE, "/bkirunge/test.txt", null) ==
                 ACLPermissionUtil.ACL_WRITE);

        // List bucket ACL
      assertTrue(ACLPermissionUtil.getACLPermission(HttpMethod.GET,
                                                    "/bkirunge?acl", null) ==
                 ACLPermissionUtil.ACL_READ_ACP);

        // Get Object ACL
      assertTrue(ACLPermissionUtil.getACLPermission(
                     HttpMethod.GET, "/bkirunge/test.txt?acl", null) ==
                 ACLPermissionUtil.ACL_READ_ACP);

        // Add bucket ACL
      assertTrue(ACLPermissionUtil.getACLPermission(HttpMethod.PUT,
                                                    "/bkirunge?acl", null) ==
                 ACLPermissionUtil.ACL_WRITE_ACP);

        // Add object ACL
      assertTrue(ACLPermissionUtil.getACLPermission(
                     HttpMethod.PUT, "/bkirunge/test.txt?acl", null) ==
                 ACLPermissionUtil.ACL_WRITE_ACP);

        // Post object ACL
      assertTrue(ACLPermissionUtil.getACLPermission(
                     HttpMethod.POST, "/bkirunge/test.txt?acl", null) ==
                 ACLPermissionUtil.ACL_WRITE_ACP);

        // Empty URI
      assertTrue(ACLPermissionUtil.getACLPermission(HttpMethod.PUT, "", null) ==
                 null);

        // Empty null URI
      assertTrue(ACLPermissionUtil.getACLPermission(HttpMethod.PUT, null,
                                                    null) == null);

        // Unknown REST method
      assertTrue(ACLPermissionUtil.getACLPermission(HttpMethod.TRACE, "/",
                                                    null) == null);

        // Unknown REST method
      assertTrue(ACLPermissionUtil.getACLPermission(HttpMethod.PATCH, "/",
                                                    null) == null);
    }

}


