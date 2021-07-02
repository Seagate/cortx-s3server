#!/usr/bin/env bash

set -ex

s3bench_path=${s3bench_path:-"../s3bench/s3bench"}
addb_path=${addb_path:-"/var/log/addb_res/"}

s3bench_ext=""
clients=0
size=""
reads=0


while [ "$1" != "" ]; do
    case "$1" in
        --s3bench_path ) shift;
                         s3bench_path=$(realpath -e "$1")
                         ;;
        --addb_path ) shift;
                      addb_path="$1"
                      ;;
        --s3bench_ext ) shift;
                        s3bench_ext="$1"
                        ;;
        -c ) shift;
             clients=$1
             ;;
        -s ) shift;
             size=$1
             ;;
        -r ) shift;
             reads=$1
             ;;
        * ) echo "Invalid argument passed";
            exit 1
            ;;
    esac
    shift
done

addb_path=$(realpath -e "$addb_path")
dt=$(date +"%s")
test_name="addb_c${clients}_s${size}_cnt${reads}_d${dt}"

test_dir="$addb_path/$test_name"
mkdir -p "$test_dir"

s3b="$s3bench_path -endpoint https://s3.seagate.com -label $test_name -skipCleanup -numClients $clients -numSamples $clients -objectSize $size -sampleReads $reads $s3bench_ext"

./jenkins-build.sh --skip_build --skip_tests --restart_haproxy --fake_obj --fake_kvs

eval "$s3b"

./jenkins-build.sh --cleanup_only --remove_m0trace --collect_addb "$test_dir"
mv "./s3bench-${test_name}.log" "$test_dir"
