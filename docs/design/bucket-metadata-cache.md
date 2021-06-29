# Bucket metadata cache

Contents

- [Bucket metadata cache](#bucket-metadata-cache)
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

This document briefly describes proposed change - cache for bucket metadata.

Querying of bucket metadata is a necessary and frequent operation that takes a lot of time.

Caching of bucket metadata will greatly reduce the time of some operations, which will be especially noticeable for small objects or with a high load of data storage operations.

The implementation is offered as standard, using chain-of-responsibility pattern. The code that needs the bucket metadata will access the cache, which, in turn, will, if necessary, access the data store.

![Bucket metadata cache](http://www.plantuml.com/plantuml/proxy?cache=no&src=https://raw.githubusercontent.com/Seagate/cortx-s3server/main/docs/sequencediagrams/bucket_metadata_cache_seq.plantuml)
