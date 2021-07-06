#!/usr/bin/env bash

# Set `addb_dir` to the required output dir
# Set `s3bpath` to the s3bench executable binary
# Set `test_plan` to the required test plan in form of json just like in example
# Run `./run_addb_batch.sh`

addb_dir="/var/log/addb_res/"
s3bpath="../s3bench/s3bench"
test_plan=$(cat <<-EOF
[
{"clients": 1, "size": "100b", "reads": 10000},
{"clients": 5, "size": "10Kb", "reads": 10000}]
EOF
         )

#################################
set -ex

s3bpath=$(realpath -e "$s3bpath")
mkdir -p "$addb_dir"

tests_num=$(echo "$test_plan" |  jq -r ". | length")

ext="-o ${addb_dir}/test_report.csv -t csv"

for i in $(seq 0 $(( tests_num - 1 )))
do
    clients=$(echo "$test_plan" |  jq -r ".[$i].clients")
    size=$(echo "$test_plan" |  jq -r ".[$i].size")
    reads=$(echo "$test_plan" |  jq -r ".[$i].reads")

    echo "i=$i clients=$clients size=$size reads=$reads"
    (time ./run_addb_test.sh --s3bench_path "$s3bpath" --addb_path "$addb_dir" -c "$clients" -s "$size" -r "$reads" --s3bench_ext "$ext")  2>&1 | tee -a "$addb_dir/workload.log"

    ext="-o ${addb_dir}/test_report.csv -t csv+"
done
