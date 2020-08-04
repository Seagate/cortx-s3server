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

package com.seagates3.jcloudclient;

import com.google.common.hash.HashCode;
import com.google.common.hash.Hashing;
import com.google.common.io.ByteSource;
import org.jclouds.io.Payload;
import org.jclouds.io.PayloadSlicer;
import org.jclouds.io.internal.BasePayloadSlicer;
import org.jclouds.io.payloads.FilePayload;

import java.io.ByteArrayOutputStream;
import java.io.File;

public class EtagGenerator {

    private File file;
    /**
     * mpuSizeMB - multipart chunk size in MB
     */
    private long mpuSizeMB;

    public EtagGenerator(File file, long mpuSizeMB) {
        this.file = file;
        this.mpuSizeMB = mpuSizeMB;
    }

    private String generateEtag() {
        FilePayload payload = new FilePayload(file);
        PayloadSlicer slicer = new BasePayloadSlicer();
        long chunkSize = mpuSizeMB * 1024L * 1024L;
        int partNumber = 0;

        ByteArrayOutputStream output = new ByteArrayOutputStream();
        for (Payload part: slicer.slice(payload, chunkSize)) {
            ByteSource source = (ByteSource) part.getRawContent();
            try {
                HashCode md5 = source.hash(Hashing.md5());
                output.write(md5.asBytes());
                partNumber++;
            } catch (Exception e) {
                e.printStackTrace();
            }
        }
        byte[] bytes = output.toByteArray();
        String eTag = Hashing.md5().hashBytes(bytes) + "-" + partNumber;

        return eTag;
    }

    public String getEtag() {
        return generateEtag();
    }
}
