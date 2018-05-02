# TODO remove this line after statsd is installed from yum
%define _unpackaged_files_terminate_build 0

# mero version
%define h_mero_version %(rpm -q --queryformat '%{VERSION}-%{RELEASE}' mero)

Name:       s3server
Version:    1.0.0
Release:    1%{?dist}
Summary:    s3server for Mero

Group:      Development/Tools
License:    Apache
URL:        http://gerrit-sage.dco.colo.seagate.com:8080/s3server
Source0:    %{name}-%{version}.tar.gz

BuildRequires: automake
BuildRequires: bazel
BuildRequires: cmake >= 2.8.12
BuildRequires: libtool
BuildRequires: mero mero-devel
BuildRequires: openssl-devel
BuildRequires: java-1.8.0-openjdk
BuildRequires: java-1.8.0-openjdk-devel
BuildRequires: maven
BuildRequires: libxml2 libxml2-devel
BuildRequires: libyaml libyaml-devel
BuildRequires: yaml-cpp yaml-cpp-devel
BuildRequires: gflags gflags-devel
BuildRequires: glog glog-devel
BuildRequires: gtest gtest-devel
BuildRequires: gmock gmock-devel
BuildRequires: git

Requires: mero = %{h_mero_version}
Requires: libxml2
Requires: libyaml
Requires: yaml-cpp
Requires: gflags
Requires: glog
Requires: pkgconfig
# Java used by Auth server
Requires: java-1.8.0-openjdk-headless

%description
S3 server provides S3 REST API interface support for Mero object storage.

%prep
%setup -q

%build
./rebuildall.sh --no-check-code --no-install

%install
rm -rf %{buildroot}
./installhelper.sh %{buildroot}

%clean
bazel clean
cd auth
mvn clean
cd ..
rm -rf %{buildroot}

%files
%defattr(-,root,root,-)
%config(noreplace) /opt/seagate/auth/resources/authserver.properties
%config(noreplace) /opt/seagate/auth/resources/static/saml-metadata.xml
%config(noreplace) /opt/seagate/s3/conf/s3config.yaml
%config(noreplace) /opt/seagate/s3/conf/s3_obj_layout_mapping.yaml
%config(noreplace) /opt/seagate/s3/conf/s3stats-whitelist.yaml
%config(noreplace) /opt/seagate/s3/statsd/s3statsd-config.js

%dir /opt/seagate/
%dir /opt/seagate/auth
%dir /opt/seagate/auth/resources
%dir /opt/seagate/auth/resources/static
%dir /opt/seagate/s3
%dir /opt/seagate/s3/bin
%dir /opt/seagate/s3/conf
%dir /opt/seagate/s3/libevent
%dir /opt/seagate/s3/libevent/pkgconfig
%dir /opt/seagate/s3/nodejs
%dir /opt/seagate/s3/resources
%dir /opt/seagate/s3/statsd
%dir /opt/seagate/s3/statsd/backends
%dir /opt/seagate/s3/statsd/bin
%dir /opt/seagate/s3/statsd/debian
%dir /opt/seagate/s3/statsd/docs
%dir /opt/seagate/s3/statsd/examples
%dir /opt/seagate/s3/statsd/examples/Etsy
%dir /opt/seagate/s3/statsd/examples/go
%dir /opt/seagate/s3/statsd/lib
%dir /opt/seagate/s3/statsd/packager
%dir /opt/seagate/s3/statsd/servers
%dir /opt/seagate/s3/statsd/test
%dir /opt/seagate/s3/statsd/utils
%dir /var/log/seagate/
%dir /var/log/seagate/auth
%dir /var/log/seagate/s3
/lib/systemd/system/s3authserver.service
/lib/systemd/system/s3server@.service
/lib/systemd/system/s3statsd.service
/opt/seagate/auth/AuthServer-1.0-0.jar
/opt/seagate/auth/resources/s3_auth.jks
/opt/seagate/auth/resources/signin.seagate.com.crt
/opt/seagate/auth/resources/iam.seagate.com.crt
/opt/seagate/auth/startauth.sh
/opt/seagate/s3/bin/cloviskvscli
/opt/seagate/s3/bin/s3server
/opt/seagate/s3/libevent/libevent-2.0.so.5
/opt/seagate/s3/libevent/libevent-2.0.so.5.1.10
/opt/seagate/s3/libevent/libevent.a
/opt/seagate/s3/libevent/libevent.la
/opt/seagate/s3/libevent/libevent.so
/opt/seagate/s3/libevent/libevent_core-2.0.so.5
/opt/seagate/s3/libevent/libevent_core-2.0.so.5.1.10
/opt/seagate/s3/libevent/libevent_core.a
/opt/seagate/s3/libevent/libevent_core.la
/opt/seagate/s3/libevent/libevent_core.so
/opt/seagate/s3/libevent/libevent_extra-2.0.so.5
/opt/seagate/s3/libevent/libevent_extra-2.0.so.5.1.10
/opt/seagate/s3/libevent/libevent_extra.a
/opt/seagate/s3/libevent/libevent_extra.la
/opt/seagate/s3/libevent/libevent_extra.so
/opt/seagate/s3/libevent/libevent_openssl-2.0.so.5
/opt/seagate/s3/libevent/libevent_openssl-2.0.so.5.1.10
/opt/seagate/s3/libevent/libevent_openssl.a
/opt/seagate/s3/libevent/libevent_openssl.la
/opt/seagate/s3/libevent/libevent_openssl.so
/opt/seagate/s3/libevent/libevent_pthreads-2.0.so.5
/opt/seagate/s3/libevent/libevent_pthreads-2.0.so.5.1.10
/opt/seagate/s3/libevent/libevent_pthreads.a
/opt/seagate/s3/libevent/libevent_pthreads.la
/opt/seagate/s3/libevent/libevent_pthreads.so
/opt/seagate/s3/libevent/pkgconfig/libevent.pc
/opt/seagate/s3/libevent/pkgconfig/libevent_openssl.pc
/opt/seagate/s3/libevent/pkgconfig/libevent_pthreads.pc
/opt/seagate/s3/resources/s3_error_messages.json
/opt/seagate/s3/s3startsystem.sh
/opt/seagate/s3/s3stopsystem.sh
/opt/seagate/s3/statsd/CONTRIBUTING.md
/opt/seagate/s3/statsd/Changelog.md
/opt/seagate/s3/statsd/LICENSE
/opt/seagate/s3/statsd/README.md
/opt/seagate/s3/statsd/backends/console.js
/opt/seagate/s3/statsd/backends/graphite.js
/opt/seagate/s3/statsd/backends/repeater.js
/opt/seagate/s3/statsd/bin/statsd
/opt/seagate/s3/statsd/debian/changelog
/opt/seagate/s3/statsd/debian/compat
/opt/seagate/s3/statsd/debian/control
/opt/seagate/s3/statsd/debian/copyright
/opt/seagate/s3/statsd/debian/dirs
/opt/seagate/s3/statsd/debian/docs
/opt/seagate/s3/statsd/debian/localConfig.js
/opt/seagate/s3/statsd/debian/postinst
/opt/seagate/s3/statsd/debian/postrm
/opt/seagate/s3/statsd/debian/proxyConfig.js
/opt/seagate/s3/statsd/debian/rules
/opt/seagate/s3/statsd/debian/statsd-proxy.upstart
/opt/seagate/s3/statsd/debian/statsd.install
/opt/seagate/s3/statsd/debian/statsd.statsd-proxy.service
/opt/seagate/s3/statsd/debian/statsd.statsd.service
/opt/seagate/s3/statsd/debian/statsd.upstart
/opt/seagate/s3/statsd/docs/admin_interface.md
/opt/seagate/s3/statsd/docs/backend.md
/opt/seagate/s3/statsd/docs/backend_interface.md
/opt/seagate/s3/statsd/docs/cluster_proxy.md
/opt/seagate/s3/statsd/docs/graphite.md
/opt/seagate/s3/statsd/docs/graphite_pickle.md
/opt/seagate/s3/statsd/docs/metric_types.md
/opt/seagate/s3/statsd/docs/namespacing.md
/opt/seagate/s3/statsd/docs/server.md
/opt/seagate/s3/statsd/docs/server_interface.md
/opt/seagate/s3/statsd/exampleConfig.js
/opt/seagate/s3/statsd/exampleProxyConfig.js
/opt/seagate/s3/statsd/examples/Etsy/StatsD.pm
/opt/seagate/s3/statsd/examples/README.md
/opt/seagate/s3/statsd/examples/StatsD.scala
/opt/seagate/s3/statsd/examples/StatsdClient.java
/opt/seagate/s3/statsd/examples/StatsdClient.jl
/opt/seagate/s3/statsd/examples/csharp_example.cs
/opt/seagate/s3/statsd/examples/go/README.md
/opt/seagate/s3/statsd/examples/go/doc.go
/opt/seagate/s3/statsd/examples/go/statsd.go
/opt/seagate/s3/statsd/examples/perl-example.pl
/opt/seagate/s3/statsd/examples/php-example.php
/opt/seagate/s3/statsd/examples/python_example.py
/opt/seagate/s3/statsd/examples/ruby_example.rb
/opt/seagate/s3/statsd/examples/ruby_example2.rb
/opt/seagate/s3/statsd/examples/statsd-client.sh
/opt/seagate/s3/statsd/examples/statsd.clj
/opt/seagate/s3/statsd/examples/statsd.erl
/opt/seagate/s3/statsd/lib/config.js
/opt/seagate/s3/statsd/lib/helpers.js
/opt/seagate/s3/statsd/lib/logger.js
/opt/seagate/s3/statsd/lib/mgmt_console.js
/opt/seagate/s3/statsd/lib/mgmt_server.js
/opt/seagate/s3/statsd/lib/process_metrics.js
/opt/seagate/s3/statsd/lib/process_mgmt.js
/opt/seagate/s3/statsd/lib/set.js
/opt/seagate/s3/statsd/package.json
/opt/seagate/s3/statsd/packager/Procfile
/opt/seagate/s3/statsd/packager/postinst
/opt/seagate/s3/statsd/proxy.js
/opt/seagate/s3/statsd/run_tests.sh
/opt/seagate/s3/statsd/servers/tcp.js
/opt/seagate/s3/statsd/servers/udp.js
/opt/seagate/s3/statsd/stats.js
/opt/seagate/s3/statsd/test/graphite_delete_counters_tests.js
/opt/seagate/s3/statsd/test/graphite_legacy_tests.js
/opt/seagate/s3/statsd/test/graphite_legacy_tests_statsprefix.js
/opt/seagate/s3/statsd/test/graphite_pickle_tests.js
/opt/seagate/s3/statsd/test/graphite_tests.js
/opt/seagate/s3/statsd/test/graphite_tests_statsprefix.js
/opt/seagate/s3/statsd/test/graphite_tests_statssuffix.js
/opt/seagate/s3/statsd/test/helpers_tests.js
/opt/seagate/s3/statsd/test/mgmt_console_tests.js
/opt/seagate/s3/statsd/test/process_metrics_tests.js
/opt/seagate/s3/statsd/test/process_mgmt_tests.js
/opt/seagate/s3/statsd/test/repeater_tests.js
/opt/seagate/s3/statsd/test/server_tests.js
/opt/seagate/s3/statsd/test/set_tests.js
/opt/seagate/s3/statsd/utils/check_statsd.pl
/opt/seagate/s3/statsd/utils/check_statsd_health
/opt/seagate/s3/statsd/utils/statsd-timer-metric-counts.sh

%post
systemctl daemon-reload
systemctl enable s3authserver
