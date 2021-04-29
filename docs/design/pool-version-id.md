# Pool version Id

Contents

- [Pool version Id](#pool-version-id)
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

This document briefly describes proposed change - pool version id usage.

Pool version is an object in configuration DB of Motr, which describes a subset of cluster.
It contains disks, controllers of disks and racks for controllers.
Cluster can be displayed as a tree with physical disks as leafs.
A pool version is a subtree of that tree.
Any Motr objects and indices are created and stored in a single pool version which is selected when an entity is creating.
Pool version Id is the identifier of a pool version.

An entity (object or index) is identified by an unique identifier. This identifier is represented as 16 bytes with almost arbitrary content. Hence, an identifier doesn't contain any hints about physical location of entity's data.
Motr implementation have to make lookup for the pool version the entity is stored in.
This lookup can be omitted if s3server reads and saves pool version id after an entity is created and then fills appropriate field(s) in control structures for futher operations with the entity.