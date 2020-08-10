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
import org.junit.Test;
import org.junit.runner.RunWith;
import org.powermock.api.mockito.mockpolicies.Slf4jMockPolicy;
import org.powermock.core.classloader.annotations.MockPolicy;
import org.powermock.core.classloader.annotations.PrepareForTest;
import org.powermock.modules.junit4.PowerMockRunner;

import java.io.BufferedWriter;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileWriter;
import java.io.IOException;

import static org.mockito.Matchers.anyString;
import static org.mockito.Mockito.verify;
import static org.powermock.api.mockito.PowerMockito.mock;
import static org.powermock.api.mockito.PowerMockito.mockStatic;
import static org.powermock.api.mockito.PowerMockito.when;
import static org.powermock.api.mockito.PowerMockito.whenNew;

@RunWith(PowerMockRunner.class)
@PrepareForTest({AuthServerConfig.class, File.class, S3Perf.class})
@MockPolicy(Slf4jMockPolicy.class)
public class S3PerfTest {

    private File file;
    private FileWriter fileWriter;
    private BufferedWriter bufferedWriter;
    private S3Perf s3Perf;

    @Test
    public void initTest() throws Exception {
        initTestHelper();
        whenNew(FileWriter.class).withArguments(file, true).thenReturn(fileWriter);
        whenNew(BufferedWriter.class).withArguments(fileWriter).thenReturn(bufferedWriter);

        S3Perf.init();

        verify(file).exists();
    }

    @Test(expected = ServerInitialisationException.class)
    public void initTest_FileNotFoundException() throws Exception {
        initTestHelper();
        whenNew(FileWriter.class).withArguments(file, true).thenThrow(FileNotFoundException.class);

        S3Perf.init();
    }

    @Test(expected = ServerInitialisationException.class)
    public void initTest_IOException() throws Exception {
        initTestHelper();
        whenNew(FileWriter.class).withArguments(file, true).thenThrow(IOException.class);

        S3Perf.init();
    }

    private void initTestHelper() throws Exception {
        mockStatic(AuthServerConfig.class);

        file = mock(File.class);
        fileWriter = mock(FileWriter.class);
        bufferedWriter = mock(BufferedWriter.class);

        when(AuthServerConfig.getPerfLogFile()).thenReturn("/var/log/seagate/auth/perf.log");
        when(AuthServerConfig.isPerfEnabled()).thenReturn(true);
        whenNew(File.class).withArguments("/var/log/seagate/auth/perf.log").thenReturn(file);
        when(file.getAbsoluteFile()).thenReturn(file);
    }

    @Test
    public void cleanTest() throws Exception {
        initTestHelper();
        whenNew(FileWriter.class).withArguments(file, true).thenReturn(fileWriter);
        whenNew(BufferedWriter.class).withArguments(fileWriter).thenReturn(bufferedWriter);
        S3Perf.init();

        S3Perf.clean();

        verify(bufferedWriter).close();
    }

    @Test
    public void printTimeTest() throws Exception {
        initTestHelper();
        whenNew(FileWriter.class).withArguments(file, true).thenReturn(fileWriter);
        whenNew(BufferedWriter.class).withArguments(fileWriter).thenReturn(bufferedWriter);
        S3Perf.init();

        s3Perf = new S3Perf();
        s3Perf.startClock();
        s3Perf.endClock();
        s3Perf.printTime("SampleMessage:");

        verify(bufferedWriter).write(anyString());
        verify(bufferedWriter).flush();
    }
}