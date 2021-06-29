# Authentication and authorization

Contents

- [Authentication and authorization](#authentication-and-authorization)
  - [License](#license)
  - [Proposed Design Change](#proposed-design-change)


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


## Proposed Design Change

This document briefly describes proposed change -- combined Auth request for both authentication and authorization.

With current implementation, s3server makes two requests to Auth server:

* user authentication (are user credentials valid),
* authorization (check, whether the user has appropriate rights for a requested operation).

See the diagram below:

![Auth Sequence: separate calls](http://www.plantuml.com/plantuml/proxy?cache=no&src=https://raw.githubusercontent.com/Seagate/cortx-s3server/main/docs/sequencediagrams/auth-separate-calls.plantuml)

Each separate Auth call adds HTTP overhead and round-trip latency.  Plus the overhead of asynchronous request processing in s3server and in Auth server.

It's suggested to change the flow, so that s3server will only make one request to Auth server.  The request will be made right after retrieving  bucket and object metadata.  It will decrease CPU usage and latency, and will improve performance, especially for small files.

![Auth Sequence: combined call](http://www.plantuml.com/plantuml/proxy?cache=no&src=https://raw.githubusercontent.com/Seagate/cortx-s3server/main/docs/sequencediagrams/auth-combined-call.plantuml)

The only known downside of the combinded approach is -- in an "unhappy" path, when user credentials are invalid, "combined call" will first load all metadata and then won't need it, which is avoided "separate calls" approach (Authentication call will return FAIL, and we won't even attempt KVS operations).  But we assume that "unhappy path" is much less frequent than "happy", so this optimization is net-positive.

"Pure" AuthenticateUser/AuthorizeUser will not be removed, as they are needed for certain operations:

* Pure authentication requests will be made for S3 background services. For such casees authorization is made by s3server itself (no need for Auth call).
* Pure authorization will be made for anonymous requests.
* One special case is copy-object operations when s3server should make at least two requests to Auth server anyway. The first request is for destination object. The second request is for source object. It's suggested to convert the first authorization request into combined.
