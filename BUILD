
cc_binary(
    # How to run build
    # bazel build //:s3server  --define MERO_SRC=`pwd`/../..

    name = "s3server",

    srcs = glob(["server/*.cc", "server/*.c"]),

    copts = [" -std=c++11 -DEVHTP_HAS_C99 -DEVHTP_SYS_ARCH=64 -DGCC_VERSION=4002 -DHAVE_CONFIG_H -DM0_TARGET=ClovisTest -D_REENTRANT -D_GNU_SOURCE -DM0_INTERNAL= -DM0_EXTERN=extern -iquote $(MERO_SRC) -iquote . -include config.h -Ithird_party/lustre-2.5.1-headers/libcfs/include -I third_party/lustre-2.5.1-headers/lnet/include -Ithird_party/lustre-2.5.1-headers/lustre/include -fno-common -Wall -Wno-attributes -fno-strict-aliasing -fno-omit-frame-pointer  -ggdb3 -O3 -DNDEBUG"],

    includes = ["third_party/libevent/s3_dist/include/",
                "third_party/googletest/include/",
                "third_party/libevhtp/s3_dist/include",
                "third_party/jsoncpp/dist",
                "third_party/yaml-cpp/include/",
                "third_party/libxml2/s3_dist/include/libxml2"],

    linkopts = ["-L $(MERO_SRC)/mero/.libs",
                "-L $(MERO_SRC)/extra-libs/gf-complete/src/.libs/", "-Lthird_party/libevent/s3_dist/lib/",
                "-Lthird_party/libevhtp/s3_dist/lib",
                "-Lthird_party/libxml2/s3_dist/lib third_party/libevhtp/s3_dist/lib/libevhtp.a",
                "-levent -levent_pthreads -levent_openssl -lssl -lcrypto",
                "-lpthread -ldl -lrt",
                "-lmero -lgf_complete -lm -lpthread -laio -lrt ",
                "-lyaml -lyaml-cpp -luuid -pthread -lxml2 -lpthread", "-Wl,-rpath,/usr/local/lib64,-rpath,/opt/seagate/s3/lib,-rpath,/opt/seagate/s3/libevent,-rpath,/opt/seagate/s3/libyaml-cpp/lib,-rpath,/opt/seagate/s3/libxml2/lib"],
)

cc_test(
    # How to run build
    # bazel build //:s3ut  --define MERO_SRC=`pwd`/../..
    # bazel test //:s3ut  --define MERO_SRC=`pwd`/../..

    name = "s3ut",

    srcs = glob(["ut/*.cc", "server/*.cc", "server/*.c"],
                exclude = ["server/s3server.cc"]),

    copts = ["-DEVHTP_DISABLE_REGEX  -std=c++11 -DEVHTP_HAS_C99 -DEVHTP_SYS_ARCH=64 -DGCC_VERSION=4002 -DHAVE_CONFIG_H -DM0_TARGET=ClovisTest -D_REENTRANT -D_GNU_SOURCE -DM0_INTERNAL= -DM0_EXTERN=extern -iquote $(MERO_SRC) -pie"],

    includes = ["third_party/googletest/include",
                "third_party/googlemock/include",
                "third_party/libevent/s3_dist/include/",
                "third_party/libevhtp/s3_dist/include",
                "third_party/jsoncpp/dist",
                "third_party/yaml-cpp/include/",
                "third_party/libxml2/s3_dist/include/libxml2",
                "server/"],

    linkopts = ["-L $(MERO_SRC)/mero/.libs",
                "-L $(MERO_SRC)/extra-libs/gf-complete/src/.libs/",
                "-Lthird_party/libevent/s3_dist/lib/",
                "-Lthird_party/libevhtp/s3_dist/lib",
                "-Lthird_party/libxml2/s3_dist/lib third_party/libevhtp/s3_dist/lib/libevhtp.a",
                "-levent -levent_pthreads -levent_openssl -lssl -lcrypto",
                "-lpthread -ldl -lrt",
                "-lmero -lgf_complete -lm -lpthread -laio -lrt ",
                "-lyaml -lyaml-cpp -luuid -pthread -lxml2 -lpthread",
                "-pthread third_party/googletest/build/libgtest.a third_party/googlemock/build/libgmock.a",
                "-Wl,-rpath,/usr/local/lib64,-rpath,$(MERO_SRC)/mero/.libs,-rpath,$(MERO_SRC)/extra-libs/gf-complete/src/.libs,-rpath,third_party/libevent/s3_dist/lib,-rpath,third_party/libxml2/s3_dist/lib,-rpath,/opt/seagate/s3/libyaml-cpp/lib"],

    data = ["resources"],
)
