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

package com.seagates3.perf;

import com.seagates3.authserver.AuthServerConfig;
import com.seagates3.exception.ServerInitialisationException;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileWriter;
import java.io.IOException;
import java.io.UnsupportedEncodingException;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class S3Perf {

    private static String FILE_NAME;
    private static File file;
    private static boolean PERF_ENABLED;

    private long startTime;
    private long endTime;

    private static FileWriter fw;
    private static BufferedWriter bw;

    private static final Logger logger
            = LoggerFactory.getLogger(S3Perf.class.getName());

    private static synchronized void writeToFile(String msg) {
        if (bw != null) {
            try {
                bw.write(msg);
                bw.flush();
            } catch (Exception ex) {
                logger.error(ex.getMessage());
            }
        }
    }

    public static void init() throws ServerInitialisationException {
        FILE_NAME = AuthServerConfig.getPerfLogFile();
        PERF_ENABLED = AuthServerConfig.isPerfEnabled();

        if (PERF_ENABLED) {
            try {
                file = new File(FILE_NAME);
                if (!file.exists()) {
                    file.createNewFile();
                }
                fw = new FileWriter(file.getAbsoluteFile(), true);
                bw = new BufferedWriter(fw);
            } catch (FileNotFoundException | UnsupportedEncodingException ex) {
                throw new ServerInitialisationException(ex.getMessage());
            } catch (IOException ex) {
                throw new ServerInitialisationException(ex.getMessage());
            }
        }
    }

    public static synchronized void clean() {
        try {
            if (bw != null) {
                bw.close();
            }
        } catch (IOException ex) {
        }
    }

    public void startClock() {
        startTime = System.currentTimeMillis();
    }

    public void printTime(String msg) {
        if (PERF_ENABLED) {
            String msgToPrint = msg + "," + (endTime - startTime) + " ms\n";
            S3Perf.writeToFile(msgToPrint);
        }
    }

    public void endClock() {
        endTime = System.currentTimeMillis();
    }
}
