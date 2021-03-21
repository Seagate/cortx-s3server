export LD_LIBRARY_PATH="$(pwd)/third_party/motr/motr/.libs/:"\
"$(pwd)/third_party/motr/helpers/.libs/:"\
"$(pwd)/third_party/motr/extra-libs/gf-complete/src/.libs/"

./bazel-bin/s3ut
