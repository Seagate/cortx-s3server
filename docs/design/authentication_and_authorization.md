# Authentication and authorization

## License

Copyright (c) 2020 Seagate Technology LLC and/or its Affiliates

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

   <http://www.apache.org/licenses/LICENSE-2.0>

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

For any questions about this software or licensing,
please email opensource@seagate.com or cortx-questions@seagate.com.

## Description

This document briefly describes combined request for both authentication and authorization.

Previously, s3server made 2 requests to Auth server: one for user authentication and one for authorization (check, whether the user has appropriate rights for a requested operation).

Now, in most cases, s3server makes only one request to Auth server. Request is made just after retrieving of bucket (and object) metadata. It decreases latency and improves performance, especially for small files.

Pure authentication is made only for requests, made by S3 background services.

Pure authorization is made for anonymous requests. One special case is copy-object operations. s3server makes 2 requests to Auth server. The first is combined request for authentication and verifying permissions for destination object. The second request is pure authorization request for source object.

