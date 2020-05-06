FROM docker.pkg.github.com/seagate/mero/custom-mero-base-image:latest

COPY scripts/docker/s3server-devel/s3rpm.spec .

RUN yum install -y yum-priorities

RUN wget -q --auth-no-challenge --header='Accept:application/octet-stream' https://8f3ee7ec7da1ce1d6791032dd0edb77b015c9892: \
@api.github.com/repos/seagate/mero/releases/assets/20476936 -O eos-core-1.0.0-1_git4da149d_3.10.0_1062.el7.x86_64.rpm

RUN wget -q --auth-no-challenge --header='Accept:application/octet-stream' https://8f3ee7ec7da1ce1d6791032dd0edb77b015c9892: \
@api.github.com/repos/seagate/mero/releases/assets/20476939 -O eos-core-devel-1.0.0-1_git4da149d_3.10.0_1062.el7.x86_64.rpm

RUN wget -q --auth-no-challenge --header="Accept:application/octet-stream" https://8f3ee7ec7da1ce1d6791032dd0edb77b015c9892:@api.github.com/repos/seagate/s3server/releases/assets/19870493 -O bazel-0.11.1-1.el7.centos.x86_64.rpm \
  && wget -q --auth-no-challenge --header="Accept:application/octet-stream" https://8f3ee7ec7da1ce1d6791032dd0edb77b015c9892:@api.github.com/repos/seagate/s3server/releases/assets/20431622 -O git-clang-format-6.0-1.el7.centos.src.rpm \
  && wget -q --auth-no-challenge --header="Accept:application/octet-stream" https://8f3ee7ec7da1ce1d6791032dd0edb77b015c9892:@api.github.com/repos/seagate/s3server/releases/assets/19868054 -O git-clang-format-6.0-1.el7.centos.x86_64.rpm \
  && wget -q --auth-no-challenge --header="Accept:application/octet-stream" https://8f3ee7ec7da1ce1d6791032dd0edb77b015c9892:@api.github.com/repos/seagate/s3server/releases/assets/20431620 -O gmock-1.7.0-1.el7.centos.src.rpm \
  && wget -q --auth-no-challenge --header="Accept:application/octet-stream" https://8f3ee7ec7da1ce1d6791032dd0edb77b015c9892:@api.github.com/repos/seagate/s3server/releases/assets/20441294 -O gmock-1.7.0-1.el7.centos.x86_64.rpm \
  && wget -q --auth-no-challenge --header="Accept:application/octet-stream" https://8f3ee7ec7da1ce1d6791032dd0edb77b015c9892:@api.github.com/repos/seagate/s3server/releases/assets/20441295 -O gmock-debuginfo-1.7.0-1.el7.centos.x86_64.rpm \
  && wget -q --auth-no-challenge --header="Accept:application/octet-stream" https://8f3ee7ec7da1ce1d6791032dd0edb77b015c9892:@api.github.com/repos/seagate/s3server/releases/assets/20441296 -O gmock-devel-1.7.0-1.el7.centos.x86_64.rpm \
  && wget -q --auth-no-challenge --header="Accept:application/octet-stream" https://8f3ee7ec7da1ce1d6791032dd0edb77b015c9892:@api.github.com/repos/seagate/s3server/releases/assets/20441298 -O gtest-1.7.0-2.el7.centos.src.rpm \
  && wget -q --auth-no-challenge --header="Accept:application/octet-stream" https://8f3ee7ec7da1ce1d6791032dd0edb77b015c9892:@api.github.com/repos/seagate/s3server/releases/assets/20441301 -O gtest-1.7.0-2.el7.centos.x86_64.rpm \
  && wget -q --auth-no-challenge --header="Accept:application/octet-stream" https://8f3ee7ec7da1ce1d6791032dd0edb77b015c9892:@api.github.com/repos/seagate/s3server/releases/assets/20441302 -O gtest-debuginfo-1.7.0-2.el7.centos.x86_64.rpm \
  && wget -q --auth-no-challenge --header="Accept:application/octet-stream" https://8f3ee7ec7da1ce1d6791032dd0edb77b015c9892:@api.github.com/repos/seagate/s3server/releases/assets/20441304 -O gtest-devel-1.7.0-2.el7.centos.x86_64.rpm \
  && wget -q --auth-no-challenge --header="Accept:application/octet-stream" https://8f3ee7ec7da1ce1d6791032dd0edb77b015c9892:@api.github.com/repos/seagate/s3server/releases/assets/19870505 -O log4cxx_eos-0.10.0-1.x86_64.rpm \
  && wget -q --auth-no-challenge --header="Accept:application/octet-stream" https://8f3ee7ec7da1ce1d6791032dd0edb77b015c9892:@api.github.com/repos/seagate/s3server/releases/assets/19870507 -O log4cxx_eos-debuginfo-0.10.0-1.x86_64.rpm \
  && wget -q --auth-no-challenge --header="Accept:application/octet-stream" https://8f3ee7ec7da1ce1d6791032dd0edb77b015c9892:@api.github.com/repos/seagate/s3server/releases/assets/19870511 -O log4cxx_eos-devel-0.10.0-1.x86_64.rpm \
  && wget -q --auth-no-challenge --header="Accept:application/octet-stream" https://8f3ee7ec7da1ce1d6791032dd0edb77b015c9892:@api.github.com/repos/seagate/s3server/releases/assets/20441307 -O ossperf-3.0-1.src.rpm \
  && wget -q --auth-no-challenge --header="Accept:application/octet-stream" https://8f3ee7ec7da1ce1d6791032dd0edb77b015c9892:@api.github.com/repos/seagate/s3server/releases/assets/19870512 -O ossperf-3.0-1.x86_64.rpm \
  && wget -q --auth-no-challenge --header="Accept:application/octet-stream" https://8f3ee7ec7da1ce1d6791032dd0edb77b015c9892:@api.github.com/repos/seagate/s3server/releases/assets/20441332 -O python-boto3-1.4.6-1.el7.centos.src.rpm \
  && wget -q --auth-no-challenge --header="Accept:application/octet-stream" https://8f3ee7ec7da1ce1d6791032dd0edb77b015c9892:@api.github.com/repos/seagate/s3server/releases/assets/20441335 -O python-botocore-1.6.0-1.el7.centos.src.rpm \
  && wget -q --auth-no-challenge --header="Accept:application/octet-stream" https://8f3ee7ec7da1ce1d6791032dd0edb77b015c9892:@api.github.com/repos/seagate/s3server/releases/assets/20441347 -O python-jmespath-0.9.0-1.el7.centos.src.rpm \
  && wget -q --auth-no-challenge --header="Accept:application/octet-stream" https://8f3ee7ec7da1ce1d6791032dd0edb77b015c9892:@api.github.com/repos/seagate/s3server/releases/assets/20441349 -O python-s3transfer-0.1.10-1.el7.centos.src.rpm \
  && wget -q --auth-no-challenge --header="Accept:application/octet-stream" https://8f3ee7ec7da1ce1d6791032dd0edb77b015c9892:@api.github.com/repos/seagate/s3server/releases/assets/20441288 -O python-wheel-0.24.0-2.el7.centos.src.rpm \
  && wget -q --auth-no-challenge --header="Accept:application/octet-stream" https://8f3ee7ec7da1ce1d6791032dd0edb77b015c9892:@api.github.com/repos/seagate/s3server/releases/assets/20441290 -O python-xmltodict-0.9.0-1.el7.centos.src.rpm \
  && wget -q --auth-no-challenge --header="Accept:application/octet-stream" https://8f3ee7ec7da1ce1d6791032dd0edb77b015c9892:@api.github.com/repos/seagate/s3server/releases/assets/20190432 -O python34-boto3-1.4.6-1.el7.centos.noarch.rpm \
  && wget -q --auth-no-challenge --header="Accept:application/octet-stream" https://8f3ee7ec7da1ce1d6791032dd0edb77b015c9892:@api.github.com/repos/seagate/s3server/releases/assets/20441310 -O python34-botocore-1.6.0-1.el7.centos.noarch.rpm \
  && wget -q --auth-no-challenge --header="Accept:application/octet-stream" https://8f3ee7ec7da1ce1d6791032dd0edb77b015c9892:@api.github.com/repos/seagate/s3server/releases/assets/20441316 -O python34-jmespath-0.9.0-1.el7.centos.noarch.rpm \
  && wget -q --auth-no-challenge --header="Accept:application/octet-stream" https://8f3ee7ec7da1ce1d6791032dd0edb77b015c9892:@api.github.com/repos/seagate/s3server/releases/assets/20441318 -O python34-s3transfer-0.1.10-1.el7.centos.noarch.rpm \
  && wget -q --auth-no-challenge --header="Accept:application/octet-stream" https://8f3ee7ec7da1ce1d6791032dd0edb77b015c9892:@api.github.com/repos/seagate/s3server/releases/assets/20441320 -O python34-wheel-0.24.0-2.el7.centos.noarch.rpm \
  && wget -q --auth-no-challenge --header="Accept:application/octet-stream" https://8f3ee7ec7da1ce1d6791032dd0edb77b015c9892:@api.github.com/repos/seagate/s3server/releases/assets/20441321 -O python34-xmltodict-0.9.0-1.el7.centos.noarch.rpm \
  && wget -q --auth-no-challenge --header="Accept:application/octet-stream" https://8f3ee7ec7da1ce1d6791032dd0edb77b015c9892:@api.github.com/repos/seagate/s3server/releases/assets/20441322 -O python36-boto3-1.4.6-1.el7.centos.noarch.rpm \
  && wget -q --auth-no-challenge --header="Accept:application/octet-stream" https://8f3ee7ec7da1ce1d6791032dd0edb77b015c9892:@api.github.com/repos/seagate/s3server/releases/assets/20441323 -O python36-botocore-1.6.0-1.el7.centos.noarch.rpm \
  && wget -q --auth-no-challenge --header="Accept:application/octet-stream" https://8f3ee7ec7da1ce1d6791032dd0edb77b015c9892:@api.github.com/repos/seagate/s3server/releases/assets/20441326 -O python36-jmespath-0.9.0-1.el7.centos.noarch.rpm \
  && wget -q --auth-no-challenge --header="Accept:application/octet-stream" https://8f3ee7ec7da1ce1d6791032dd0edb77b015c9892:@api.github.com/repos/seagate/s3server/releases/assets/20441328 -O python36-s3transfer-0.1.10-1.el7.centos.noarch.rpm \
  && wget -q --auth-no-challenge --header="Accept:application/octet-stream" https://8f3ee7ec7da1ce1d6791032dd0edb77b015c9892:@api.github.com/repos/seagate/s3server/releases/assets/20441329 -O python36-wheel-0.24.0-2.el7.centos.noarch.rpm \
  && wget -q --auth-no-challenge --header="Accept:application/octet-stream" https://8f3ee7ec7da1ce1d6791032dd0edb77b015c9892:@api.github.com/repos/seagate/s3server/releases/assets/20441330 -O python36-xmltodict-0.9.0-1.el7.centos.noarch.rpm \
  && wget -q --auth-no-challenge --header="Accept:application/octet-stream" https://8f3ee7ec7da1ce1d6791032dd0edb77b015c9892:@api.github.com/repos/seagate/s3server/releases/assets/20441291 -O s3cmd-1.6.1-1.el7.centos.noarch.rpm

RUN rpm --install git-clang-format-6.0-1.el7.centos.src.rpm gmock-1.7.0-1.el7.centos.src.rpm gtest-1.7.0-2.el7.centos.src.rpm \
       ossperf-3.0-1.src.rpm python-boto3-1.4.6-1.el7.centos.src.rpm python-botocore-1.6.0-1.el7.centos.src.rpm \
       python-jmespath-0.9.0-1.el7.centos.src.rpm python-s3transfer-0.1.10-1.el7.centos.src.rpm python-wheel-0.24.0-2.el7.centos.src.rpm \
       python-xmltodict-0.9.0-1.el7.centos.src.rpm
RUN yum install -y eos-core-1.0.0-1_git4da149d_3.10.0_1062.el7.x86_64.rpm \
               eos-core-devel-1.0.0-1_git4da149d_3.10.0_1062.el7.x86_64.rpm \
    && yum install -y bazel-0.11.1-1.el7.centos.x86_64.rpm git-clang-format-6.0-1.el7.centos.x86_64.rpm \
           log4cxx_eos-0.10.0-1.x86_64.rpm log4cxx_eos-debuginfo-0.10.0-1.x86_64.rpm log4cxx_eos-devel-0.10.0-1.x86_64.rpm \
           gmock-1.7.0-1.el7.centos.x86_64.rpm gmock-debuginfo-1.7.0-1.el7.centos.x86_64.rpm gmock-devel-1.7.0-1.el7.centos.x86_64.rpm \
           gtest-1.7.0-2.el7.centos.x86_64.rpm gtest-debuginfo-1.7.0-2.el7.centos.x86_64.rpm gtest-devel-1.7.0-2.el7.centos.x86_64.rpm \
           ossperf-3.0-1.x86_64.rpm python36-boto3-1.4.6-1.el7.centos.noarch.rpm python36-botocore-1.6.0-1.el7.centos.noarch.rpm \
           python36-jmespath-0.9.0-1.el7.centos.noarch.rpm python36-s3transfer-0.1.10-1.el7.centos.noarch.rpm python36-wheel-0.24.0-2.el7.centos.noarch.rpm \
           python36-xmltodict-0.9.0-1.el7.centos.noarch.rpm s3cmd-1.6.1-1.el7.centos.noarch.rpm \
    && yum-builddep -y --define 'python3_other_pkgversion 36' s3rpm.spec

RUN rm -f eos-core-1.0.0-1_git4da149d_3.10.0_1062.el7.x86_64.rpm \
           eos-core-devel-1.0.0-1_git4da149d_3.10.0_1062.el7.x86_64.rpm \
           bazel-0.11.1-1.el7.centos.x86_64.rpm git-clang-format-6.0-1.el7.centos.x86_64.rpm \
           log4cxx_eos-0.10.0-1.x86_64.rpm log4cxx_eos-debuginfo-0.10.0-1.x86_64.rpm log4cxx_eos-devel-0.10.0-1.x86_64.rpm \
           gmock-1.7.0-1.el7.centos.x86_64.rpm gmock-debuginfo-1.7.0-1.el7.centos.x86_64.rpm gmock-devel-1.7.0-1.el7.centos.x86_64.rpm \
           gtest-1.7.0-2.el7.centos.x86_64.rpm gtest-debuginfo-1.7.0-2.el7.centos.x86_64.rpm \
           ossperf-3.0-1.x86_64.rpm python34-boto3-1.4.6-1.el7.centos.noarch.rpm python34-botocore-1.6.0-1.el7.centos.noarch.rpm \
           python34-jmespath-0.9.0-1.el7.centos.noarch.rpm python34-s3transfer-0.1.10-1.el7.centos.noarch.rpm python34-wheel-0.24.0-2.el7.centos.noarch.rpm \
           python34-xmltodict-0.9.0-1.el7.centos.noarch.rpm python36-boto3-1.4.6-1.el7.centos.noarch.rpm python36-botocore-1.6.0-1.el7.centos.noarch.rpm \
           python36-jmespath-0.9.0-1.el7.centos.noarch.rpm python36-s3transfer-0.1.10-1.el7.centos.noarch.rpm python36-wheel-0.24.0-2.el7.centos.noarch.rpm \
           python36-xmltodict-0.9.0-1.el7.centos.noarch.rpm s3cmd-1.6.1-1.el7.centos.noarch.rpm \
           git-clang-format-6.0-1.el7.centos.src.rpm gmock-1.7.0-1.el7.centos.src.rpm gtest-1.7.0-2.el7.centos.src.rpm \
           ossperf-3.0-1.src.rpm python-boto3-1.4.6-1.el7.centos.src.rpm python-botocore-1.6.0-1.el7.centos.src.rpm \
           python-jmespath-0.9.0-1.el7.centos.src.rpm python-s3transfer-0.1.10-1.el7.centos.src.rpm python-wheel-0.24.0-2.el7.centos.src.rpm \
           python-xmltodict-0.9.0-1.el7.centos.src.rpm \
           && yum remove -y mero mero-devel
