# Full object protection (R1)

Feature is also nick-named as DI -- data integrity.

## Description

This feature is about calculating checksums for entire S3 objects.

- S3 PUT
  - MD5 checksum is calculated for each object. ETag from S3 client is checked
    against calculated checksum and the PUT request fails if there is a
    mismatch. The checksum is saved during save_metadata.
- S3 multipart upload
  - MD5 checksum is calculated independently for each part and for each part
    ETag from client is checked against calculated checksum (the same as for
    PUT).
  - S3AwsEtag class is used during CompleteMultipartUpload to calculate ETag
    for entire object.
- S3 GET
  - For an object that had been uploaded using S3 PUT ETag from the object
    metadata against MD5 checksum calculated during object I/O.
  - For an object that had been uploaded using multipart upload Etag is
    calculated in the same way as it was calculated in CompleteMultipartUpload.
  - If there is a mismatch between calculated Etag and Etag from the object
    metadata, then S3 server cancels HTTP request before returning the last
    portion of data to the client.  An IEM alert is also raised, to be
    consumed by other components (e.g. HA) as an indicator of an integrity
    issue.

## Enable/Disable the feature

Feature is controlled by two options in `s3config.yaml` file, under
section `S3_SERVER_CONFIG`:

- `S3_READ_MD5_CHECK_ENABLED` -- when true, activates MD5 validation during
  GET object call.  When false, no validation.
- `S3_RANGED_READ_ENABLED` -- when true, range read requests is enabled.

Note: range read is now coupled with DI check feature, reason -- R1
implementation uses md5 checksum for entire object content, and it is not
possible/reasonable to validate "partial" read, i.e. when user requests
only part of an object.  So -- if DI is enabled, we also must disable
range read for consistency.  These are left as separate options, because
developers (and cluster admins) may choose to enable it explicitly even
when DI is active for some reasons.

So, to enable DI (default for production config):

```
S3_READ_MD5_CHECK_ENABLED: true
S3_RANGED_READ_ENABLED: false
```

To disable DI (default for dev/test config):

```
S3_READ_MD5_CHECK_ENABLED: false
S3_RANGED_READ_ENABLED: true
```

Note: we make it disabled by default for dev environment because range reads
is a valid feature and we need it available in dev env by default.
