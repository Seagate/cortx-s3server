# ADDB

For reference see:
- cortx-s3server/addb/readme
- cortx-s3server/addb/addb-py/readme - outdated, but gives brief descriptions of steps
- cortx-s3server/server/addb-codegen.py - called from jenkins-build to regenerate mapping
- cortx-s3server/server/s3_addb*.[ch]

## Motr is always run with enabled addb

## S3server

add `--addb` command line flag to startup command

```
s3server --s3pidfile $pid_filename \
    --motrlocal $local_ep:${motr_local_port} --motrha $ha_ep \
    --s3port $s3port --fault_injection true $fake_params --loading_indicators --getoid true \
    --addb
```

## Collect addb

Notes:

- motr and s3server addb are stored separatelly
    - s3server on dev env: `/var/log/seagate/motr/s3server-*/addb_*/o/100000000000000:2`
    - motr on dev env: `/var/motr/systest-*/ios*/addb-stobs-*/o/100000000000000:2`

- all motr and s3 addb are collected on individual process basis on each node
- process PID is a part of name
- during the conversion from bin to text, process PID HAVE TO BE SAVED as a part of a filename. Processing scripts assume special filename format
    - filename format: `<name>_PID.txt` where `<name>` should not contain `-` or `_`
    - suggested name for motr is `dumps_<PID>.txt`
    - suggested name for s3 is `dumpc_<PID>.txt`

- s3server addb plugin - `/opt/seagate/cortx/s3/addb-plugin/libs3addbplugin.so` - should only be used with the version of s3serve it was build for
- `m0addb2dump` is build and deployed with mero

Steps:

- stop ALL s3server services
- stop ALL motr services
- collect motr addb on dev env

```
cd ./third_party/motr/addb2/
# motr addb
dump_s="/var/motr/systest-*/ios*/addb-stobs-*/o/100000000000000:2"
for d in $dump_s; do
    pid=$(echo $d | sed -E 's/.*addb-stobs-([a-z0-9]*)[/].*/\1/')
    echo 'mero '${pid}
    ./m0addb2dump -f -- "$d" > "$collect_addb/dumps_${pid}.txt"
done
cd -
```

- collect s3server addb on dev env

```
cd ./third_party/motr/addb2/
# s3server addb
dump_s="/var/log/seagate/motr/s3server-*/addb_*/o/100000000000000:2"
for d in $dump_s; do
    pid=$(echo $d | sed -E 's/.*addb_([0-9]+)[/].*/\1/')
    echo 's3server '${pid}
    ./m0addb2dump -f -p /opt/seagate/cortx/s3/addb-plugin/libs3addbplugin.so -- "$d" > "$collect_addb/dumpc_${pid}.txt"
done
cd -
```

- where `$collect_addb` is output folder
- in case of deployed cluster addb should be collected for each node
- during the process lots of m0trace files could be created. it is better to delete them - they require lots of disk space

## Processing

Addb tools repo: `https://github.com/Seagate/seagate-tools`
Addb scripts: `https://github.com/Seagate/seagate-tools/tree/main/performance/PerfLine/roles/perfline_setup/files/chronometry_v2`
Mainteiner: Alexander Sukhachev
Knows a lot: Maxim Malezhin
Also: Ivan Tishchenko, Anatoliy Bilenko

- convert text to db

```
#!/usr/bin/env bash
set -ex
script_dir=seagate-tools/performance/PerfLine/roles/perfline_setup/files/chronometry_v2
raw_data_dir=addb_res    # path to folder with test results
raw_data_dir=$(realpath -e $raw_data_dir)
cd $script_dir
for d in $(ls -d $raw_data_dir/addb_c*); do
    test_name=$(echo $d | sed -E 's/.*addb_(c[0-9]+_s[0-9]+[mMkKgGbB]+_cnt[0-9]+_d[0-9]+)/\1/')
    echo "$d :> $test_name"
    ./addb2db_multiprocess.sh --dumps $d/dump*.txt
    cp ./m0play.db $d/m0play_${test_name}.db
    rm -rf ./m0play.db
done
cd -
```

- example of `raw_data_dir` folder structure
    - folder `raw_data_dir`
        - subfolder `addb_c1_s16Mb_cnt1000`
            - content `dumps_<PID>.txt`
            - content `dumpc_<PID>.txt`
        - subfolder `addb_c22_s512Kb_cnt1000`
            - content `dumps_<PID>.txt`
            - content `dumpc_<PID>.txt`

- generate histograms

```
./hist.py -t s3_request_state -u ms -r 2 [[Action::check_authentication,S3Action::load_metadata],[S3Action::load_metadata,S3Action::check_authorization],[S3Action::check_authorization,S3GetObjectAction::validate_object_info]] -o hist_auth.png -f png -s 20 10 --db m0play_4k_auth.db
```

detailed description of all params could be found in `./hist.py --help`

- draw timeline

```
python3.6 req_timelines.py -p 191874 13 -e 1
```

where 191874 is a pid of the s3server instance, 13 is a request id that should be visualized,
1 is a number of lines that should be displayed. Each line is logical level of requet processing,
e.g. s3server, clovis, dix, fom, etc

pid and request id could be found in m0play.db file - result of the convertion of text addb files to database.
`sqlite3` utility can be used to analize the database with help of SQL requests.

- it is possible to use GNUPlot to draw timelines. Example `cortx-experiments/S3/addb-low-write-256kb`


# S3bench

Repo: `github.com/Seagate/s3bench.git`
Building: install golang and run `build.sh`. for the first build it might be required to run `build.sh` twice

## Implemented tests

- Put object
- Multipart upload
- Copy object
- Put obj tag
- Get obj tag
- Get object
- Head object
- Validate written objects

## Workflow

- parse params
- check bucket, create if not exists
- prepare data
- run workload
- delete data, if not set to skip
- generate report

## Object naming

`<prefix>_<hash>_<index>`

where:

- `<prefix>` is set on cmd with `-objectNamePrefix` parameter
- `<hash>` is a base32 encoded sha512 checksum of the object, generated by s3bench
- `<index>` is a simple zero base counter, for copies it consists of 2 parts, added by s3bench

note:

`<hash>` is used for content validation

Example:
- test_FQV3BH4KSJWK6PQUK3KOYE6WEP3HRDJ5YG5V7WODEKSJGXR6IYOO2FESHBQKORNPJ3ZLELBCGHUCTQFYMTEIDQMDXO5PBWM3GXJCTTA_0
- test_FQV3BH4KSJWK6PQUK3KOYE6WEP3HRDJ5YG5V7WODEKSJGXR6IYOO2FESHBQKORNPJ3ZLELBCGHUCTQFYMTEIDQMDXO5PBWM3GXJCTTA_0_1

## s3bench_hash

`s3bench_hash` folder contains source code for the utility that allows to calculate `<hash>` for a specified file.

```
bash-4.2$ ./s3bench_hash -file ./s3bench_hash.go
File ./s3bench_hash.go
Size 1042
S3Bench Hash FQV3BH4KSJWK6PQUK3KOYE6WEP3HRDJ5YG5V7WODEKSJGXR6IYOO2FESHBQKORNPJ3ZLELBCGHUCTQFYMTEIDQMDXO5PBWM3GXJCTTA
```

It could be built with `build.sh`.

## Command line parameters

### Version

- `-version` - print version info and exit

Version is generated during compilation and consists of date it was built and
git commit hash it was built from

```
bash-4.2$ ./s3bench -version
2021-04-29-10:03:53-92f326e72ef46660787978ad7028ea28341f467a
```

### Credentials

#### Explicit key/secret

- `-accessKey` - S3 access key
- `-accessSecret` - S3 access secret

#### AWS credentials

s3bench can use aws utility credential file - `~/.aws/credentials`.
if no credential parameters provided s3bench will use `default` profile from `~/.aws/credentials`.

Different profile could be used via command line parameter `-profile` - name of the required profile.

### AWS S3 settings

- `-endpoint` - S3 endpoint(s) comma separated - http://IP:PORT,http://IP:PORT
- `-region` - AWS region to use, eg: us-west-1|us-east-1, etc
- `-bucketName` - the bucket for which to run the test

### Network settings

s3bench uses Amazon AWS SDK for go v1. Following parameters are directly mapped to corresponding AWS SDK parametrs.
Additional information could be found in documentation for Amazon AWS SDK for go v1

- `-s3MaxRetries` - The maximum number of times that a request will be retried for failures. Defaults to -1, which defers the max retry setting to the service specific configuration
- `-s3Disable100Continue` - Set this to `true` to disable the SDK adding the `Expect: 100-Continue` header to PUT requests over 2MB of content. 100-Continue instructs the HTTP client not to send the body until the service responds with a `continue` status. This is useful to prevent sending the request body until after the request is authenticated, and validated. You should use this flag to disable 100-Continue if you experience issues with proxies or third party S3 compatible services
- `-httpClientTimeout` - Specifies a time limit in Milliseconds for requests made by this Client. The timeout includes connection time, any redirects, and reading the response body. The timer remains running after Get, Head, Post, or Do return and will interrupt reading of the Response.Body. A Timeout of zero means no timeout
- `-connectTimeout` - Maximum amount of time a dial will wait for a connect to complete. The default is no timeout
- `-TLSHandshakeTimeout` - Specifies the maximum amount of time waiting to wait for a TLS handshake. Zero means no timeout
- `-maxIdleConnsPerHost` - If non-zero, controls the maximum idle (keep-alive) connections to keep per-host. If zero, system default value is used
- `-idleConnTimeout` - Max amount of time in Milliseconds an idle (keep-alive) connection will remain idle before closing itself. Zero means no limit
- `-responseHeaderTimeout` - If non-zero, specifies the amount of time in Milliseconds to wait for a server's response headers after fully writing the request (including its body, if any). This time does not include the time to read the response body

### General settings

- `-protocolDebug` - Trace http headers during client-server exchange
- `-label` - Assign unique label for test. Subst %d in provided label for current date and %t for current timestamp
- `-numClients` - number of concurrent clients for specified workloads
- `-numSamples` - base number of objects to operate in workloads. Total number of requests could be different for different tests

### Object settings

- `-objectNamePrefix` - prefix of the object name that will be used
- `-objectSize` - size of each object to operate on. Must be smaller than main memory. Format is
`<num><size>`, where `<num>` is positive int and `<size>` is a string one of ["b", "Kb", "Mb", "Gb"]
- `-zero` - fill object content with all zeroes. By default object are created in memory and filled by random data from `/dev/urandom`, but for large objects, e.g. 50Gb, it could take some time. To speed up the test it is possible to create objects filled with zeros

### Output settings

#### Logger

Each s3bench run creates a log file. It contains verbose output of all actions done during the workload,
detailed non-filtered report and, if enabled, client-server communication.

#### Report

- `-outstream` - Path to output report
- `-outtype` - Type of the report. Should be one of [txt, json, csv, csv+]
- `-reportFormat` - Rearrange output fields

Formats:

- `txt` - simple text form, without any structure
- `json` - in form of unindented json
- `csv` - dump column names and then tests results
- `csv+` - append test results to the file whitout adding column names

##### CSV report

Several tests with the same set of tests and the same report formatting could be
combined into single csv report.

First test is run with `-outtype` set to `csv`. The other tests are run with the same
`-outstream` and `-outtype` set to `csv+`. In such a configuration first test will
create column name and the other tests will be appended in the end of file. The resulting
csv report will contain test result from all runs

The set of fields in report and its order are set by `-reportFormat`

#### reportFormat syntax

There are two section in report: parameters and tests.
Parameters describe all the settings s3bench were run with. Tests describes the measured values.
Each reporting field in report is described with full specification:`<section>:<field>`.
Where `<section>` is `Parameters` or `Tests` and filed is reporting value. Reporting fields
are splitted with `;` in `-reportFormat`. If field description starts with `-` this field will be
excluded from report. The order of fields in report is the order of fields descriptions in `-reportFormat`.

Default value:
`Parameters:label;Parameters:numClients;Parameters:objectSize (MB);Parameters:copies;-Parameters:numSamples;-Parameters:sampleReads;-Parameters:readObj;-Parameters:headObj;-Parameters:putObjTag;-Parameters:getObjTag;-Parameters:TLSHandshakeTimeout;-Parameters:bucket;-Parameters:connectTimeout;-Parameters:deleteAtOnce;-Parameters:deleteClients;-Parameters:deleteOnly;-Parameters:endpoints;-Parameters:httpClientTimeout;-Parameters:idleConnTimeout;-Parameters:maxIdleConnsPerHost;-Parameters:multipartSize;-Parameters:numTags;-Parameters:objectNamePrefix;-Parameters:protocolDebug;-Parameters:reportFormat;-Parameters:responseHeaderTimeout;-Parameters:s3Disable100Continue;-Parameters:profile;-Parameters:s3MaxRetries;-Parameters:skipRead;-Parameters:skipWrite;-Parameters:tagNamePrefix;-Parameters:tagValPrefix;-Parameters:validate;-Parameters:zero;-Parameters:outstream;-Parameters:outtype;Tests:Operation;Tests:RPS;Tests:Total Requests Count;Tests:Errors Count;Tests:Total Throughput (MB/s);Tests:Total Duration (s);Tests:Total Transferred (MB);Tests:Duration Max;Tests:Duration Avg;Tests:Duration Min;Tests:Ttfb Max;Tests:Ttfb Avg;Tests:Ttfb Min;-Tests:Duration 25th-ile;-Tests:Duration 50th-ile;-Tests:Duration 75th-ile;-Tests:Ttfb 25th-ile;-Tests:Ttfb 50th-ile;-Tests:Ttfb 75th-ile;-Tests:Errors;-Version;`

### Write tests

- `-skipWrite` - do not run put-object test. Does not affect `-multipartSize`
- `-multipartSize` - run MultipartUpload with specified part size instead of put-object. Part size has the same format as `-objectSize`.
- `-copies` - copy each written object specified number of times. put-object or multipart upload workload must be specified. The other enabled ops will also include copies into processing

### Read tests

- `-skipRead` - do not run get-object test
- `-sampleReads` - read each object specified number of times
- `-validate` - validate stored data. each written object is read, content checksum is calculated and compared with checksum stored in object name

### Metadata tests

- `-headObj` - run head-object requests tests
- `-putObjTag` - add specified number of tags with specified names and values to each object
- `-getObjTag` - get object's tags. if `-getObjTag` specified `-putObjTag` test will added into workload automatically
- `-numTags` - number of tags to create. for objects it should be in range [1..10]
- `-tagNamePrefix` - prefix of the tag name that will be used. full tag's name is `<tagNamePrefix><index>`
- `-tagValPrefix` - prefix of the tag value that will be used, full tag's value is `<tagValPrefix><index>`

### Delete settings

Delete phase consists of the following steps:

- delete all tags if they were added during the tests
- delete all objects
- delete bucket if it were created during the test

Objects are deleted with batches. Batches could be send in parallel.

- `-skipCleanup` - skip delete phase
- `-deleteOnly` - only run delete phase. the other tests are not run
- `-deleteAtOnce` - number of objs to delete at one batch
- `-deleteClients` - number of concurrent clients to send delete requests


# jenkins-build

List of some features implemented in jenkins-build

- skiping different steps
    - skiping build: `./jenkins-build.sh --skip_build`
    - skiping different types or tests: ` ./jenkins-build.sh --skip_tests --skip_ut_run --skip_st_run`
- delete all data and temp files: `./jenkins-build.sh --cleanup_only && rm -rf /var/motr`
- run several instancies of s3server: `./jenkins-build.sh --num_instances N`
- run basic tests: `./jenkins-build.sh --basic_test_only`.  with help of s3cmd run following scenario
    - Create Buckets
    - List Buckets
    - Put Object of various sizes
    - Get Object the uploaded objects
    - Delete Object
    - Delete Bucket
- generate valgrind memleak reports: `./jenkins-build.sh --valgrind_memcheck /path/to/output/file`
- generate valgrind callgraph:  `./jenkins-build.sh --callgraph /path/to/output/file`
- callgraph and memleak could be used with basic tests
- run s3server with fake io/kvs: `./jenkins-build.sh --fake_obj --fake_kvs`
- run s3server with redis kvs: `./jenkins-build.sh --local_redis_restart --redis_kvs`
