This page is using a 3rd-party server to automatically render plantuml images which allow github markdown to use text-based diagrams which are stored in a separate file. Note that as of February 2022, an even better solution is possible which is that mermaid text-based diagrams can be embedded directly into the markdown. So, this new approach eliminates the need for a separate file and for a 3rd-party server. Here is a very simple example:
```mermaid
flowchart LR
A[Hard] -->|Text| B(Round)
B --> C{Decision}
C -->|One| D[Result 1]
C -->|Two| E[Result 2]
```

![CORTX S3 Simple Object Upload](http://www.plantuml.com/plantuml/proxy?cache=no&src=https://raw.githubusercontent.com/Seagate/cortx-s3server/main/docs/sequencediagrams/s3_metadata_struct.plantuml)

![CORTX S3 Delete Multiple Objects (Batch Delete)](http://www.plantuml.com/plantuml/proxy?cache=no&src=https://raw.githubusercontent.com/Seagate/cortx-s3server/main/docs/sequencediagrams/delete-multiple-objects-activity.plantuml)
