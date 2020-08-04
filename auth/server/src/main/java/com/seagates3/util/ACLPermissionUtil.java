/*
 * Copyright (c) 2020 Seagate Technology LLC and/or its Affiliates
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * For any questions about this software or licensing,
 * please email opensource@seagate.com or cortx-questions@seagate.com.
 *
 */

package com.seagates3.util;

import java.util.HashMap;

import io.netty.handler.codec.http.HttpMethod;

/*
 * Utility class to get ACL permission type given method and request uri
 */

public class ACLPermissionUtil {

    public static final String ACL_READ = "READ";
    public static final String ACL_READ_ACP = "READ_ACP";
    public static final String ACL_WRITE = "WRITE";
    public static final String ACL_WRITE_ACP = "WRITE_ACP";
    public static final String ACL_FULL_CONTROL = "FULL_CONTROL";

    static final String ACL_QUERY = "?acl";

    static final HashMap<HttpMethod, String> permissionMap = initPermissionMap();

    private static HashMap<HttpMethod, String> initPermissionMap() {

        HashMap<HttpMethod, String> pMap = new HashMap<>();
        pMap.put(HttpMethod.GET, ACL_READ);
        pMap.put(HttpMethod.HEAD, ACL_READ);
        pMap.put(HttpMethod.PUT, ACL_WRITE);
        pMap.put(HttpMethod.DELETE, ACL_WRITE);
        pMap.put(HttpMethod.POST, ACL_WRITE);
        return pMap;
    }

    final static HashMap<HttpMethod, String> permissionMapACP = initPermissionMapACP();

    private static HashMap<HttpMethod, String> initPermissionMapACP() {

        HashMap<HttpMethod, String> pMap = new HashMap<>();
        pMap.put(HttpMethod.GET, ACL_READ_ACP);
        pMap.put(HttpMethod.HEAD, ACL_READ_ACP);
        pMap.put(HttpMethod.PUT, ACL_WRITE_ACP);
        pMap.put(HttpMethod.DELETE, ACL_WRITE_ACP);
        pMap.put(HttpMethod.POST, ACL_WRITE_ACP);
        return pMap;
    }

    /*
     * Returns ACL Permission
     * @param method Request method
     * @param uri Request Uri
     * @return ACL permission and null in case of error
     */
   public
    static String getACLPermission(HttpMethod method, String uri,
                                   String queryParam) {

        if (uri == null || uri.isEmpty()) {
            IEMUtil.log(IEMUtil.Level.ERROR,
                    IEMUtil.INVALID_REST_URI_ERROR,
                            "URI is empty or null", null);
            return null;
        }
        if (isACLReadWrite(uri, queryParam)) {
           return permissionMapACP.get(method);
        }
        else {
            return permissionMap.get(method);
        }
    }

   public
    static boolean isACLReadWrite(String uri, String queryParam) {
      if (uri.endsWith(ACL_QUERY) || "acl".equals(queryParam)) {
            return true;
        }
        else {
            return false;
        }
    }
}


