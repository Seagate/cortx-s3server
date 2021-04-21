# Full object protection (R1)

Feature is also nick-named as DI -- data integrity.

## Description

This feature is about calculating checksums for entire S3 objects.

- S3 PUT
  - MD5 checksum is calculated for each object. ETag from S3 client is checked
    against calculated checksum and the PUT request fails if there is a
    mismatch. The checksum is saved during save_metadata.
  - Note: This code has been there even before implementation of DI, so no
    changes here.
- S3 multipart upload
  - MD5 checksum is calculated independently for each part and for each part
    ETag from client is checked against calculated checksum (the same as for
    PUT).
  - S3AwsEtag class is used during CompleteMultipartUpload to calculate ETag
    for entire object.
  - Note: This code has been there even before implementation of DI, so no
    changes here.
- S3 chunked upload
  - MD5 checksum is calculated for the entire object, similar to "S3 PUT'
    above.  Chunked upload API uses different content validation mechanism
    (every chunk is signed based on the results of previous chunk), so MD5
    checksum is not used for validation -- but it's now added to object
    metadata and stored permanently, so we can use it later to validate
    content in S3 GET.
  - Note: MD5 calculation was not there; DI change adds it.
- S3 GET
  - For an object that had been uploaded using S3 PUT, ETag from the object
    metadata against MD5 checksum calculated during object I/O.
  - For an object that had been uploaded using multipart upload, Etag is
    calculated in the same way as it was calculated in CompleteMultipartUpload.
  - For chunked upload, DI check is the same as for S3 PUT.
  - If there is a mismatch between calculated Etag and Etag from the object
    metadata, then S3 server cancels HTTP request before returning the last
    portion of data to the client.  An IEM alert is also raised, to be
    consumed by other components (e.g. HA) as an indicator of an integrity
    issue.
  - All these checks during GET, are new, added as part of DI implementation.

As a part of Data Integrity, S3server validates metadata stored on
persistent storage before retrieving actual object data or returning it back
to the user. This provides more strict guarantees to the user w.r.t.
not receiving corrupted, non consistent or non valid objects and helps to
detect data/metadata corruption issues earlier. If such check fails, error
is returned to the user and IEM is reported to HA indicating an appropriate
failure.


## IEM alert details

### Alert ID: 0030060004

Current version of alert text:

> Data integrity failure (content checksum mismatch).
> Cluster is transitioning to safe mode. Contact Seagate Support.

Alert also includes JSON with details on the failure -- bucket/object name,
saved/calculated checksum, and OID of the object.

### Alert ID: 0030060005

Current version of alert text:

> Metadata integrity failure (bucket/object mismatch).
> Cluster is transitioning to safe mode. Contact Seagate Support.

Alert also includes JSON with details on the failure -- bucket, object names
from the request and bucket, object names read from KVS.


## Range Read behavior

If DI check is enabled (and range reads disabled), GET Object for range read
request will return HTTP error 401, `OperationNotSupported`.

Requests which are returning HTTP header `Accept-Ranges` (like Get Object
request), before the fix were returning `Accept-Ranges: bytes`.  Now, with
DI enabled (and range reads disabled) will return `Accept-Ranges: none`.
If DI is disabled (and range reads enabled) will return
`Accept-Ranges: bytes` -- as before.


## Enable/Disable the feature

Feature is controlled by four options in `s3config.yaml` file, under
section `S3_SERVER_CONFIG`:

- `S3_READ_MD5_CHECK_ENABLED` -- when true, activates MD5 validation during
  GET object call.  When false, no validation.
- `S3_RANGED_READ_ENABLED` -- when true, range read requests is enabled.
- `S3_DI_DISABLE_DATA_CORRUPTION_IEM` -- when true, IEMs for md5 checksum
  validation mismatch are not raised.
- `S3_DI_DISABLE_METADATA_CORRUPTION_IEM` -- when true, IEMs for invalid
  metadata entries are not raised.

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


## Metadata integrity

Metadata integrity is always-on and independent from md5 checksum calculation.
The check is performed on every Object- or Part- related requests.
Such checks shouldn't be heavy therefore no performance impact is expected.
