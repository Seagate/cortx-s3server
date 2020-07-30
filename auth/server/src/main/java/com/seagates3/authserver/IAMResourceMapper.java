package com.seagates3.authserver;

import java.io.IOException;
import com.google.gson.Gson;
import com.seagates3.exception.AuthResourceNotFoundException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.UnsupportedEncodingException;
import java.util.HashMap;

/**
 * IAM APIs do not follow restful architecture. IAM requests are HTTP POST
 * requests to URL - "https://iam.seagate.com/". These requests contain a field
 * "Action" in the post body which indicates the operation to be performed. Ex -
 * Action: CreateUser. This class is written to map the IAM actions to the
 * corresponding controller and action.
 */
public class IAMResourceMapper {

    private static final String ROUTES_CONFIG_FILE = "/IAMroutes.json";

    private static HashMap<String, String> routeConfigs;

    /**
     * Read the handler mapping rules from routes.json.
     *
     * @throws java.io.UnsupportedEncodingException
     */
    public static void init() throws UnsupportedEncodingException {
       try(InputStream in =
               IAMResourceMapper.class.getResourceAsStream(ROUTES_CONFIG_FILE);
           InputStreamReader reader = new InputStreamReader(in, "UTF-8")) {
        Gson gson = new Gson();
        routeConfigs = gson.fromJson(reader, HashMap.class);
       }
       catch (IOException e) {
         // Do nothing
       }
    }

    /**
     * Get the controller and action for the request URI.
     *
     * URI should be in the format /controller/action. If a match isn't found
     * for the full URI i.e "/controller/action", then check if the resource has
     * a wild card entry for the controller i.e "/controller/*".
     *
     * @param action
     * @return ResourceMap.
     * @throws com.seagates3.exception.AuthResourceNotFoundException
     */
    public static ResourceMap getResourceMap(String action)
            throws AuthResourceNotFoundException {
        String controllerAction = routeConfigs.get(action);

        if (controllerAction == null) {
            String errorMessage = "Requested operation " + action
                    + " is not supported.";
            throw new AuthResourceNotFoundException(errorMessage);
        }

        String[] tokens = controllerAction.split("#", 2);
        return new ResourceMap(tokens[0], tokens[1]);
    }
}
