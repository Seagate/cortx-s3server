
cc_binary(
    # How to run build
    # bazel build //:s3server --cxxopt="-std=c++11" --define MERO_SRC=`pwd`/../..

    name = "s3server",

    srcs = glob(["server/*.cc", "server/*.c", "server/*.h"]),

    copts = [
      "-DEVHTP_HAS_C99", "-DEVHTP_SYS_ARCH=64", "-DGCC_VERSION=4002",
      "-DHAVE_CONFIG_H", "-DM0_TARGET=ClovisTest", "-D_REENTRANT",
      "-D_GNU_SOURCE", "-DM0_INTERNAL=", "-DM0_EXTERN=extern",
      # Do NOT change the order of strings in below line
      "-iquote", "$(MERO_SRC)", "-iquote", ".", "-include", "config.h",
      "-Ithird_party/lustre-2.5.1-headers/libcfs/include",
      "-Ithird_party/lustre-2.5.1-headers/lnet/include",
      "-Ithird_party/lustre-2.5.1-headers/lustre/include",
      "-fno-common", "-Wall", "-Wno-attributes", "-fno-strict-aliasing",
      "-fno-omit-frame-pointer", "-Werror", "-ggdb3", "-O3", "-DNDEBUG",
    ],

    includes = [
      "third_party/libevent/s3_dist/include/",
      "third_party/googletest/include/",
      "third_party/gflags/s3_dist/include/",
      "third_party/glog/s3_dist/include/",
      "third_party/libevhtp/s3_dist/include/evhtp",
      "third_party/jsoncpp/dist",
      "third_party/yaml-cpp/include/",
      "third_party/libxml2/s3_dist/include/libxml2",
    ],

    linkopts = [
      "-L $(MERO_SRC)/mero/.libs",
      "-L $(MERO_SRC)/extra-libs/gf-complete/src/.libs/",
      "-Lthird_party/libevent/s3_dist/lib/",
      "-Lthird_party/libevhtp/s3_dist/lib",
      "-Lthird_party/yaml-cpp/s3_dist/lib",
      "-Lthird_party/libxml2/s3_dist/lib",
      "-Lthird_party/gflags/s3_dist/lib",
      "-Lthird_party/glog/s3_dist/lib",
      "-levhtp -levent -levent_pthreads -levent_openssl -lssl -lcrypto",
      "-lpthread -ldl -lm -lrt -lmero -lgf_complete -laio",
      "-lyaml -lyaml-cpp -luuid -pthread -lxml2 -lgflags",
      "-pthread third_party/glog/s3_dist/lib/libglog.a",
      "-Wl,-rpath,/usr/local/lib64,-rpath,/opt/seagate/s3/lib",
      "-Wl,-rpath,/opt/seagate/s3/libevent",
      "-Wl,-rpath,/opt/seagate/s3/libyaml-cpp/lib",
      "-Wl,-rpath,/opt/seagate/s3/libxml2/lib",
    ],
)

cc_test(
    # How to run build
    # bazel build //:s3ut --cxxopt="-std=c++11" --define MERO_SRC=`pwd`/../..
    # bazel test //:s3ut --cxxopt="-std=c++11" --define MERO_SRC=`pwd`/../..

    name = "s3ut",

    srcs = glob(["ut/*.cc", "ut/*.h", "server/*.cc", "server/*.c", "server/*.h"],
                exclude = ["server/s3server.cc"]),

    copts = [
      "-DEVHTP_DISABLE_REGEX", "-DEVHTP_HAS_C99", "-DEVHTP_SYS_ARCH=64",
      "-DGCC_VERSION=4002", "-DHAVE_CONFIG_H", "-DM0_TARGET=ClovisTest",
      "-D_REENTRANT", "-D_GNU_SOURCE", "-DM0_INTERNAL=",
      "-DM0_EXTERN=extern", "-pie", "-Wno-attributes", "-O3", "-Werror",
      # Do NOT change the order of strings in below line
      "-iquote", "$(MERO_SRC)",
    ],

    includes = [
      "third_party/googletest/include",
      "third_party/googlemock/include",
      "third_party/gflags/s3_dist/include/",
      "third_party/glog/s3_dist/include/",
      "third_party/libevent/s3_dist/include/",
      "third_party/libevhtp/s3_dist/include/evhtp",
      "third_party/jsoncpp/dist",
      "third_party/yaml-cpp/include/",
      "third_party/libxml2/s3_dist/include/libxml2",
      "server/",
    ],

    linkopts = [
      "-L $(MERO_SRC)/mero/.libs",
      "-L $(MERO_SRC)/extra-libs/gf-complete/src/.libs/",
      "-Lthird_party/libevent/s3_dist/lib/",
      "-Lthird_party/libevhtp/s3_dist/lib",
      "-Lthird_party/yaml-cpp/s3_dist/lib",
      "-Lthird_party/libxml2/s3_dist/lib",
      "-Lthird_party/googletest/build",
      "-Lthird_party/googlemock/build",
      "-Lthird_party/glog/s3_dist/lib",
      "-Lthird_party/gflags/s3_dist/lib",
      "-levhtp -levent -levent_pthreads -levent_openssl -lssl -lcrypto",
      "-lpthread -ldl -lm -lrt -lmero -lgf_complete -laio",
      "-lyaml -lyaml-cpp -luuid -pthread -lxml2 -lgtest -lgmock -lgflags",
      "-pthread third_party/glog/s3_dist/lib/libglog.a",
      "-Wl,-rpath,/usr/local/lib64,-rpath,$(MERO_SRC)/mero/.libs",
      "-Wl,-rpath,$(MERO_SRC)/extra-libs/gf-complete/src/.libs",
      "-Wl,-rpath,third_party/libevent/s3_dist/lib",
      "-Wl,-rpath,third_party/libxml2/s3_dist/lib",
      "-Wl,-rpath,third_party/yaml-cpp/s3_dist/lib",
    ],

    data = [
      "resources",
    ],
)

cc_binary(
    # How to run build
    # bazel build //:s3perfclient

    name = "s3perfclient",

    srcs = glob(["perf/*.cc"]),

    copts = ["-std=c++11 -fPIC -DEVHTP_HAS_C99 -DEVHTP_SYS_ARCH=64 -O3"],

    includes = ["third_party/libevent/s3_dist/include/",
                "third_party/libevhtp/s3_dist/include/evhtp",
                "third_party/gflags/s3_dist/include/",
                "server/"],

    linkopts = ["-Lthird_party/libevent/s3_dist/lib/",
                "-Lthird_party/libevhtp/s3_dist/lib third_party/libevhtp/s3_dist/lib/libevhtp.a third_party/gflags/s3_dist/lib/libgflags.a",
                "-levent -levent_pthreads -levent_openssl -lssl -lcrypto",
                "-lpthread -ldl -lrt",
                "-Wl,-rpath,/opt/seagate/s3/lib,-rpath,/opt/seagate/s3/libevent"],
)
