package com.seagates3.util;

import com.google.gson.JsonSyntaxException;
import com.seagates3.model.SAMLMetadataTokens;
import org.junit.Test;

import static org.junit.Assert.*;

public class JSONUtilTest {

    @Test
    public void serializeToJsonTest() {
        assertNotNull(JSONUtil.serializeToJson(new Object()));
    }

    @Test
    public void deserializeFromJsonTest() {
        String jsonBody = "{username: seagate}";

        assertNotNull(JSONUtil.deserializeFromJson(jsonBody, SAMLMetadataTokens.class));
    }

    @Test
    public void deserializeFromJsonTest_ShouldReturnNullIfJsonBodyIsEmpty() {
        String jsonBody = "";
        assertNull(JSONUtil.deserializeFromJson(jsonBody, SAMLMetadataTokens.class));
    }

    @Test(expected = JsonSyntaxException.class)
    public void deserializeFromJsonTest_InvalidSyntaxException() {
        JSONUtil.deserializeFromJson("InvalidJSONBody", SAMLMetadataTokens.class);
    }
}
