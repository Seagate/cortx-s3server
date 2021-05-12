# Full object protection

Feature is also nick-named as DI -- data integrity.

## Description

The feature includes `Metadata integrity` and `Object integrity`.

### Metadata integrity description

As a part of Data Integrity, S3server validates metadata stored on
persistent storage before retrieving actual object data or returning it back
to the user. This provides more strict guarantees to the user w.r.t.
not receiving corrupted, non consistent or non valid objects and helps to
detect data/metadata corruption issues earlier. If such check fails, error
is returned to the user and IEM is reported to HA indicating an appropriate
failure.

### Objects integrity description

**TBD**


## IEM alert details

### Alert ID: 0030060004

Current version of alert text:

> Data integrity validation failure (content checksum mismatch).
> Cluster is transitioning to safe mode. Contact Seagate Support.

Alert also includes JSON with details on the failure -- bucket/object name,
saved/calculated checksum, and OID of the object.

### Alert ID: 0030060005

Current version of alert text:

> Metadata integrity validation failure (bucket/object mismatch).
> Cluster is transitioning to safe mode. Contact Seagate Support.

Alert also includes JSON with details on the failure -- bucket, object names
from the request and bucket, object names read from KVS.


## Enable/Disable the feature

### Enable Metadata integrity

Metadata integrity is always-on. The check is performed on every Object- or
Part- related requests. Such checks shouldn't be heavy therefore no performance
impact is expected.

### Enable Objects integrity

**TBD**
