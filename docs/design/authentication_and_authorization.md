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

s3server makes two requests to Auth server: one for user authentication and one for authorization (check, whether the user has appropriate rights for a requested operation).

![Authenticaion and authorization before](http://www.plantuml.com/plantuml/proxy?cache=no&src=https://raw.githubusercontent.com/Seagate/cortx-s3server/br/as/EOS-16558/docs/sequencediagrams/auth-before.plantuml)

It's suggested to change the flow, so that s3server will only make one request to Auth server. The request shall be made just after retrieving of bucket (and object) metadata. It shall decrease latency and improve performance, especially for small files.

![Authenticaion and authorization after](http://www.plantuml.com/plantuml/proxy?cache=no&src=https://raw.githubusercontent.com/Seagate/cortx-s3server/br/as/EOS-16558/docs/sequencediagrams/auth-after.plantuml)

Pure authentication requests will be made only S3 background services. For such casees authorization is made by s3server itself.

Pure authorization is made for anonymous requests.

One special case is copy-object operations when s3server should make at least two requests to Auth server anyway. The first request is for destination object. The second request is for source object. It's suggested to convert the first authorization request into combined.
