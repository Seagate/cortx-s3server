#!/bin/bash

usage() {
        USAGE="Usage:
-a : To run all the s3ut test cases.
-f : To provide gtest filter.
     Run with –f \"TestSuit.Testcase\" to run specific test suit or test case.
     Ex 1. Run specific test suit -
        runut.sh –f \"S3GetObjectActionTest.*\"

     Ex 2. Run specific test case -
        runut.sh –f \"S3GetObjectActionTest.ValidateObjectOfSizeZero\"
-h : For usage."
        echo -e "$USAGE"
	exit 0
}


filter=
extra_opts=
while getopts :fh opt; do
	case "$opt" in
	a)
		;;
	f)
		shift
		filter="$1"
		if [ -z "$filter" ]; then
			echo -e "\nMust specify some UT pattern with -f option\n"
			usage
		fi
		extra_opts="--gtest_filter=$filter"
		;;
	h)
		usage
		;;
	*)
		usage
		;;
	esac
done

export LD_LIBRARY_PATH="$(pwd)/third_party/motr/motr/.libs/:"\
"$(pwd)/third_party/motr/helpers/.libs/:"\
"$(pwd)/third_party/motr/extra-libs/gf-complete/src/.libs/:"\
"$(pwd)/third_party/motr/extra-libs/galois/src/.libs/:"

echo "Running UTs..."
./bazel-bin/s3ut "$extra_opts"
