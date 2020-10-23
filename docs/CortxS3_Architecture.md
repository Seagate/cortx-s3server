The CORTX S3 Server is the S3 compatible API to [CORTX](https://github.com/Seagate/cortx) and is 
implemented by using the [KV and object API's](https://github.com/Seagate/cortx-motr/blob/main/motr/client.h) 
to the [CORTX motr](https://github.com/Seagate/cortx-motr) 
to store and retrieve metadata and data respectively.

To learn more the CORTX S3 architecture, a good place to start is by viewing the sequence diagrams [here](sequencediagrams) 
and [here](sequencediagrams/put-object-clovis-ops).

Our S3 Compatability Matrix can be found [here](../s3-supported-api.md).

Finally, please also refer to our [Main Overview](https://github.com/Seagate/cortx/blob/main/doc/be/CORTX-S3OVERVIEW.rst) 
and our [Authentication Overview](https://github.com/Seagate/cortx/blob/main/doc/be/CORTX_S3_IAM_Overview.rst).
