package com.seagates3.util;

import com.google.gson.Gson;
import com.google.gson.GsonBuilder;

public class JSONUtil {

    /**
     * Serialize the class into a JSON String.
     *
     * @param obj
     * @return
     */
    public static String serializeToJson(Object obj) {
        Gson gson = new GsonBuilder().create();
        return gson.toJson(obj);
    }

    /**
     * Convert json to SAML Metadata Tokens object.
     *
     * @param jsonBody
     * @param deserializeClass
     * @return
     */
    public static Object deserializeFromJson(String jsonBody,
            Class<?> deserializeClass) {
        Gson gson = new Gson();

        return gson.fromJson(jsonBody, deserializeClass);
    }

}
