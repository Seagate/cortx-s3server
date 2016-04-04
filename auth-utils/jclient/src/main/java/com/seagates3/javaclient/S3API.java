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
 * Original creation date: 12-Feb-2016
 */
package com.seagates3.javaclient;

import com.amazonaws.AmazonClientException;
import com.amazonaws.ClientConfiguration;
import com.amazonaws.auth.AWSCredentials;
import com.amazonaws.services.s3.AmazonS3Client;
import com.amazonaws.services.s3.S3ClientOptions;
import com.amazonaws.services.s3.model.AbortMultipartUploadRequest;
import com.amazonaws.services.s3.model.Bucket;
import com.amazonaws.services.s3.model.CompleteMultipartUploadRequest;
import com.amazonaws.services.s3.model.DeleteObjectsRequest;
import com.amazonaws.services.s3.model.GetObjectMetadataRequest;
import com.amazonaws.services.s3.model.GetObjectRequest;
import com.amazonaws.services.s3.model.InitiateMultipartUploadRequest;
import com.amazonaws.services.s3.model.InitiateMultipartUploadResult;
import com.amazonaws.services.s3.model.ListMultipartUploadsRequest;
import com.amazonaws.services.s3.model.ListObjectsRequest;
import com.amazonaws.services.s3.model.ListPartsRequest;
import com.amazonaws.services.s3.model.MultipartUpload;
import com.amazonaws.services.s3.model.MultipartUploadListing;
import com.amazonaws.services.s3.model.ObjectListing;
import com.amazonaws.services.s3.model.ObjectMetadata;
import com.amazonaws.services.s3.model.PartETag;
import com.amazonaws.services.s3.model.PartListing;
import com.amazonaws.services.s3.model.PartSummary;
import com.amazonaws.services.s3.model.PutObjectRequest;
import com.amazonaws.services.s3.model.S3Object;
import com.amazonaws.services.s3.model.S3ObjectSummary;
import com.amazonaws.services.s3.model.UploadPartRequest;
import static com.seagates3.javaclient.ClientConfig.getClientConfiguration;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.LinkedList;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import org.apache.commons.cli.CommandLine;

public class S3API {

    private final AmazonS3Client client;
    private final CommandLine cmd;
    private String bucketName;
    private String keyName;

    public S3API(CommandLine cmd) {
        this.cmd = cmd;
        verifyCreds();

        ClientConfiguration config = getClientConfiguration();
        AWSCredentials creds;

        if (cmd.hasOption("t")) {
            creds = ClientConfig.getCreds(cmd.getOptionValue("x"),
                    cmd.getOptionValue("y"), cmd.getOptionValue("t"));
        } else {
            creds = ClientConfig.getCreds(cmd.getOptionValue("x"),
                    cmd.getOptionValue("y"));
        }

        client = new AmazonS3Client(creds, config);

        if (!cmd.hasOption("a")) {
            String endPoint;
            try {
                endPoint = ClientConfig.getEndPoint(client.getClass());
                client.setEndpoint(endPoint);
            } catch (IOException ex) {
                printError(ex.toString());
            }
        }

        S3ClientOptions clientOptions = new S3ClientOptions();
        if (cmd.hasOption("p")) {
            clientOptions.withPathStyleAccess(true);
        }

        client.setS3ClientOptions(clientOptions);
    }

    public void makeBucket() {
        checkCommandLength(2);
        getBucketObjectName(cmd.getArgs()[1]);
        if (bucketName.isEmpty()) {
            printError("Incorrect command. Bucket name is required.");
        }

        try {
            if (cmd.hasOption("l")) {
                client.createBucket(bucketName, cmd.getOptionValue("l"));
            } else {
                client.createBucket(bucketName);
            }
            System.out.println("Bucket created successfully.");
        } catch (AmazonClientException e) {
            printError(e.toString());
        }
    }

    public void removeBucket() {
        checkCommandLength(2);
        getBucketObjectName(cmd.getArgs()[1]);
        if (bucketName.isEmpty()) {
            printError("Incorrect command. Bucket name is required.");
        }

        try {
            client.deleteBucket(bucketName);
            System.out.println("Bucket deleted successfully.");
        } catch (AmazonClientException e) {
            printError(e.toString());
        }
    }

    public void list() {
        String s3Url;
        if (cmd.getArgs().length == 2) {
            s3Url = cmd.getArgs()[1];
        } else {
            s3Url = "s3://";
        }

        getBucketObjectName(s3Url);

        if (bucketName.isEmpty()) {
            try {
                List<Bucket> buckets = client.listBuckets();
                System.out.println("Buckets - ");
                for (Bucket bucket : buckets) {
                    System.out.println(bucket.getName());
                }
            } catch (AmazonClientException e) {
                printError(e.toString());
            }

            return;
        }

        try {

            ListObjectsRequest listObjectsRequest;
            if (keyName.isEmpty()) {
                listObjectsRequest = new ListObjectsRequest()
                        .withBucketName(bucketName);
            } else {
                listObjectsRequest = new ListObjectsRequest()
                        .withBucketName(bucketName)
                        .withPrefix(keyName);
            }

            ObjectListing listObjects;
            do {
                listObjects = client.listObjects(listObjectsRequest);
                for (S3ObjectSummary objectSummary
                        : listObjects.getObjectSummaries()) {
                    System.out.println(" - " + objectSummary.getKey() + "  "
                            + "(size = " + objectSummary.getSize()
                            + ")");
                }
                listObjectsRequest.setMarker(listObjects.getNextMarker());
            } while (listObjects.isTruncated());

        } catch (AmazonClientException e) {
            printError(e.toString());
        }
    }

    public void getObject() {
        checkCommandLength(3);
        if (cmd.getArgs()[2].isEmpty()) {
            printError("Give output file.");
        }

        getBucketObjectName(cmd.getArgs()[1]);

        if (bucketName.isEmpty() || keyName.isEmpty()) {
            printError("Incorrect command. Bucket name and key is required.");
        }

        try {
            S3Object object = client.getObject(
                    new GetObjectRequest(bucketName, keyName));
            InputStream inputStream = object.getObjectContent();
            File file = new File(cmd.getArgs()[2]);

            OutputStream outputStream = new FileOutputStream(file);

            int read;
            byte[] bytes = new byte[1024];

            while ((read = inputStream.read(bytes)) != -1) {
                outputStream.write(bytes, 0, read);
            }

            System.out.println("Object download successfully.");
        } catch (AmazonClientException e) {
            printError(e.toString());
        } catch (FileNotFoundException ex) {
            printError("Incorrect target file.\n" + ex.toString());
        } catch (IOException ex) {
            printError("Error occured while copying s3ObjectStream to target "
                    + "file.\n" + ex.toString());
        }
    }

    public void putObject() {
        checkCommandLength(3);
        String fileName = cmd.getArgs()[1];
        File file = new File(fileName);

        if (!file.exists()) {
            System.err.println("Given file doesn't exist.");
            System.exit(1);
        }

        getBucketObjectName(cmd.getArgs()[2]);

        if (bucketName.isEmpty()) {
            printError("Incorrect command. Bucket name is required.");
            printError("Incorrect command. Check java client usage.");
        }

        if (keyName.isEmpty()) {
            keyName = file.getName();
        } else if (keyName.endsWith("/")) {
            keyName += file.getName();
        }

        if (cmd.hasOption("m")) {
            multiPartUpload(file);
            return;
        }

        try {
            client.putObject(
                    new PutObjectRequest(bucketName, keyName, file));
            System.out.println("Object put successfully.");
        } catch (AmazonClientException e) {
            printError(e.toString());
        }
    }

    public void partialPutObject() {
        checkCommandLength(4);

        String fileName = cmd.getArgs()[1];
        File file = new File(fileName);

        if (!file.exists()) {
            System.err.println("Given file doesn't exist.");
            System.exit(1);
        }

        getBucketObjectName(cmd.getArgs()[2]);

        if (bucketName.isEmpty()) {
            printError("Incorrect command. Bucket name is required.");
            printError("Incorrect command. Check java client usage.");
        }

        if (keyName.isEmpty()) {
            keyName = file.getName();
        } else if (keyName.endsWith("/")) {
            keyName += file.getName();
        }

        int noOfParts = Integer.parseInt(cmd.getArgs()[3]);

        InitiateMultipartUploadRequest initRequest
                = new InitiateMultipartUploadRequest(bucketName, keyName);

        InitiateMultipartUploadResult initResponse = null;
        try {
            initResponse = client.initiateMultipartUpload(initRequest);
        } catch (AmazonClientException ex) {
            printError(ex.toString());
        }

        System.out.println("Upload id - " + initResponse.getUploadId());

        long contentLength = file.length();
        long partSize = Integer.parseInt(cmd.getOptionValue("m")) * 1024 * 1024;

        try {
            long filePosition = 0;
            int i = 1;
            while (i <= noOfParts && filePosition < contentLength) {
                partSize = Math.min(partSize, (contentLength - filePosition));

                UploadPartRequest uploadRequest = new UploadPartRequest()
                        .withBucketName(bucketName).withKey(keyName)
                        .withUploadId(initResponse.getUploadId())
                        .withPartNumber(i)
                        .withFileOffset(filePosition)
                        .withFile(file)
                        .withPartSize(partSize);

                System.out.println("Uploading part " + i + " of size "
                        + partSize / (1024 * 1024) + "MB");

                client.uploadPart(uploadRequest);

                filePosition += partSize;
                i++;
            }
        } catch (AmazonClientException e) {
            printError("Partial upload failed." + e.getMessage());
        }
    }

    public void deleteObject() {
        checkCommandLength(2);
        getBucketObjectName(cmd.getArgs()[1]);

        if (bucketName.isEmpty() || keyName.isEmpty()) {
            printError("Incorrect command. Check java client usage.");
            printError("Incorrect command. Bucket name and key are required.");
        }

        try {
            client.deleteObject(bucketName, keyName);
            System.out.println("Object deleted successfully.");
        } catch (AmazonClientException e) {
            printError(e.toString());
        }
    }

    public void deleteMultipleObjects() {
        if (cmd.getArgList().size() < 2) {
            printError("Incorrect command");
        }

        getBucketObjectName(cmd.getArgs()[1]);

        if (bucketName.isEmpty()) {
            printError("Incorrect command. Bucket name is required.");
        }
        String[] objects = cmd.getArgs();
        List<DeleteObjectsRequest.KeyVersion> objectList = new LinkedList<>();

        for (int index = 2; index < objects.length; index++) {
            objectList.add(new DeleteObjectsRequest.KeyVersion(objects[index]));
        }

        try {
            DeleteObjectsRequest request = new DeleteObjectsRequest(bucketName);
            request.withKeys(objectList);

            client.deleteObjects(request);
            System.out.println("Objects deleted successfully.");
        } catch (Exception e) {
            printError(e.toString());
        }
    }

    public void headObject() {
        checkCommandLength(2);
        getBucketObjectName(cmd.getArgs()[1]);

        if (bucketName.isEmpty() || keyName.isEmpty()) {
            printError("Provider bucket and object name.");
        }

        try {

            ObjectMetadata objectMetadata = client.getObjectMetadata(
                    new GetObjectMetadataRequest(bucketName, keyName));
            if (objectMetadata == null) {
                System.out.println("Object does not exist.");
            } else {
                String metadata = "Bucket name- " + bucketName;
                metadata += "\nObject name - " + keyName;
                metadata += "\nObject size - " + objectMetadata.getContentLength();
                metadata += "\nEtag - " + objectMetadata.getETag();
                metadata += "\nLastModified - " + objectMetadata.getLastModified();

                System.out.println(metadata);
            }
        } catch (Exception e) {
            printError(e.toString());
        }
    }

    public void listMultiParts() {
        checkCommandLength(2);
        getBucketObjectName(cmd.getArgs()[1]);

        if (bucketName.isEmpty()) {
            printError("Incorrect command. Bucket name is required.");
        }

        try {
            MultipartUploadListing uploads = client.listMultipartUploads(
                    new ListMultipartUploadsRequest(bucketName));

            System.out.println("Multipart uploads - ");
            for (MultipartUpload upload : uploads.getMultipartUploads()) {
                System.out.println("Name - " + upload.getKey() + ", Upload id - "
                        + upload.getUploadId());
            }
        } catch (AmazonClientException ex) {
            printError(ex.toString());
        }
    }

    public void listParts() {
        checkCommandLength(3);
        getBucketObjectName(cmd.getArgs()[1]);

        if (bucketName.isEmpty() || keyName.isEmpty()) {
            printError("Incorrect command. Bucket name and key are required.");
        }

        String uploadId = cmd.getArgs()[2];
        if (uploadId.isEmpty()) {
            printError("Provide upload id.");
        }

        try {
            PartListing listParts = client.listParts(
                    new ListPartsRequest(bucketName, keyName, uploadId)
            );

            System.out.println("Object - " + keyName);
            for (PartSummary part : listParts.getParts()) {
                System.out.println("part number - " + part.getPartNumber() + ", "
                        + " Size - " + part.getSize()
                        + " Etag - " + part.getETag());
            }
        } catch (AmazonClientException ex) {
            printError(ex.toString());
        }
    }

    public void abortMultiPart() {
        checkCommandLength(3);
        getBucketObjectName(cmd.getArgs()[1]);

        if (bucketName.isEmpty() || keyName.isEmpty()) {
            printError("Incorrect command. Bucket name and key are required.");
        }

        String uploadId = cmd.getArgs()[2];
        if (uploadId.isEmpty()) {
            printError("Provide upload id.");
        }

        try {
            client.abortMultipartUpload(new AbortMultipartUploadRequest(
                    bucketName, keyName, uploadId));

            System.out.println("Upload aborted successfully.");
        } catch (AmazonClientException ex) {
            printError(ex.toString());
        }
    }

    public void exists() {
        checkCommandLength(2);
        getBucketObjectName(cmd.getArgs()[1]);

        if (bucketName.isEmpty()) {
            printError("Bucket name cannot be empty");
        }

        if (keyName.isEmpty()) {
            try {
                if (client.doesBucketExist(bucketName)) {
                    System.out.println("Bucket " + bucketName + " exists.");
                } else {
                    System.out.println("Bucket " + bucketName
                            + " does not exist.");
                }

            } catch (AmazonClientException ex) {
                printError(ex.toString());
            }
        } else {
            try {
                /**
                 * This is a work around. The latest AWS SDK has doesObjectExist
                 * API which has to be used here.
                 */
                ObjectMetadata objectMetadata = client.getObjectMetadata(
                        new GetObjectMetadataRequest(bucketName, keyName));
                if (objectMetadata == null) {
                    System.out.println("Object does not exist.");
                } else {
                    System.out.println("Object exists.");
                }
            } catch (AmazonClientException ex) {
                System.out.println("Object does not exist.");
            }
        }
    }

    private void getBucketObjectName(String url) {
        Pattern pattern = Pattern.compile("s3://(.*?)$");
        Matcher matcher = pattern.matcher(url);
        String token = "";
        if (matcher.find()) {
            token = matcher.group(1);
        } else {
            printError("Incorrect command. Check java client usage.");
        }

        if (token.isEmpty()) {
            bucketName = "";
            keyName = "";
        } else {
            String[] tokens = token.split("/", 2);
            bucketName = tokens[0];
            if (tokens.length == 2) {
                keyName = tokens[1];
            } else {
                keyName = "";
            }
        }
    }

    private void checkCommandLength(int length) {
        if (cmd.getArgs().length < length) {
            printError("Incorrect command");
        }
    }

    private void multiPartUpload(File file) {
        List<PartETag> partETags = new ArrayList<>();

        InitiateMultipartUploadRequest initRequest
                = new InitiateMultipartUploadRequest(bucketName, keyName);

        InitiateMultipartUploadResult initResponse = null;
        try {
            initResponse = client.initiateMultipartUpload(initRequest);
        } catch (AmazonClientException ex) {
            printError(ex.toString());
        }

        System.out.println("Upload id - " + initResponse.getUploadId());

        long contentLength = file.length();
        long partSize = Integer.parseInt(cmd.getOptionValue("m")) * 1024 * 1024;

        try {
            long filePosition = 0;
            for (int i = 1; filePosition < contentLength; i++) {
                partSize = Math.min(partSize, (contentLength - filePosition));

                UploadPartRequest uploadRequest = new UploadPartRequest()
                        .withBucketName(bucketName).withKey(keyName)
                        .withUploadId(initResponse.getUploadId())
                        .withPartNumber(i)
                        .withFileOffset(filePosition)
                        .withFile(file)
                        .withPartSize(partSize);

                System.out.println("Uploading part " + i + " of size "
                        + partSize / (1024 * 1024) + "MB");
                partETags.add(client.uploadPart(uploadRequest).getPartETag());

                filePosition += partSize;
            }

            CompleteMultipartUploadRequest compRequest
                    = new CompleteMultipartUploadRequest(bucketName, keyName,
                            initResponse.getUploadId(), partETags);

            client.completeMultipartUpload(compRequest);
            System.out.println("File successfully uploaded.");
        } catch (AmazonClientException e) {
            client.abortMultipartUpload(new AbortMultipartUploadRequest(
                    bucketName, keyName, initResponse.getUploadId()));
            printError("Multipart upload failed." + e.getMessage());
        }
    }

    private void verifyCreds() {
        if (!(cmd.hasOption("x") && cmd.hasOption("y"))) {
            printError("Provide access Key and secret key");
        }
    }

    private void printError(String msg) {
        System.err.println(msg);
        System.exit(1);
    }
}
