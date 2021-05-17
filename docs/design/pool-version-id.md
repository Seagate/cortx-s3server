# Pool version Id

Contents

- [Pool version Id](#pool-version-id)
  - [Proposed Design Change](#proposed-design-change)


## Proposed Design Change

This document briefly describes proposed change - pool version id usage.

Pool version is an object in configuration DB of Motr, which describes a subset of cluster.
It contains disks, controllers of disks and racks for controllers.
Cluster can be displayed as a tree with physical disks as leafs.
A pool version is a subtree of that tree.
Any Motr objects and indices are created and stored in a single pool version which is selected when an entity is created.
Pool version Id is the identifier of a pool version.

An entity (object or index) is identified by an unique identifier. This identifier is represented as 16 bytes with almost arbitrary content. Hence, an identifier doesn't contain any hints about physical location of entity's data.
Motr implementation have to make lookup for the pool version the entity is stored in.
This lookup can be omitted if s3server reads and saves pool version id after an entity is created and then fills appropriate field(s) in control structures for futher operations with the entity.

* * *

So, every object operation done through libclovis, takes OID (object ID) as an argument.  (Except for create.)  Every operation needs to know layout ID and pool version ID (PVID) to operate on the object.  Before the fix (May 2021), S3 would only pass OID to libmotr, and so what happened -- Motr will do internal KVS lookup to load the "missing" data, layout ID and PVID.  Each object operation was doing internal KVS lookup.

But this data is "static" data (does not change ever since object is created).  More over, we already store layout ID in object metadata.

So idea is -- if we also store PVID, and then always pass these to libmotr on object operations -- no KVS lookup will be needed.  This will significantly decrease amount of KVS operations we do, and consequently improve the performance.

Notes:

* This fix depends on certain changes in Motr (before this fix, Motr was not expecting that client can pre-populate layout ID and PVID, so even if S3 provides data, Motr would ignore it).
  * But still the fix is "harmless", and can be delivered before Motr changes.  There is no "hard", or "API" dependency, all API and structures are in Motr `main` already.
* The fix is only required for "happy path" scenarios.  We assume that errors/failures/wrong API usage are rare, and we don't have to optimize those.  We only need to optimize scenarios which are "mainstream" usage, a majority of scenarios (assumes that user has all access rights, there is no internal errors that require rollback, etc.)
