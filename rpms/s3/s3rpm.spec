# mero version
%define h_mero_version %(rpm -q --queryformat '%{VERSION}-%{RELEASE}' mero)

# build number
%define build_num  %( test -n "$build_number" && echo "$build_number" || echo 1 )

Name:       s3server
Version:    %{_s3_version}
Release:    %{build_num}_%{_s3_git_ver}_%{?dist:el7}
Summary:    s3server for Mero

Group:      Development/Tools
License:    Seagate
URL:        http://gerrit.mero.colo.seagate.com:8080/s3server
Source:     %{name}-%{version}-%{_s3_git_ver}.tar.gz

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
BuildRequires: log4cxx_eos log4cxx_eos-devel


Requires: mero = %{h_mero_version}
Requires: libxml2
Requires: libyaml
Requires: yaml-cpp
Requires: gflags
Requires: glog
Requires: pkgconfig
Requires: log4cxx_eos log4cxx_eos-devel
# Java used by Auth server
Requires: java-1.8.0-openjdk-headless
Requires: PyYAML

%description
S3 server provides S3 REST API interface support for Mero object storage.

%prep
%setup -n %{name}-%{version}-%{_s3_git_ver}

%build
./rebuildall.sh --no-check-code --no-install

%install
rm -rf %{buildroot}
./installhelper.sh %{buildroot}

%clean
bazel clean
cd auth
./mvnbuild.sh clean
cd ..
rm -rf %{buildroot}

%files
%defattr(-,root,root,-)
%config(noreplace) /opt/seagate/auth/resources/authserver.properties
%config(noreplace) /opt/seagate/auth/resources/authserver-log4j2.xml
%config(noreplace) /opt/seagate/auth/resources/authencryptcli-log4j2.xml
%config(noreplace) /opt/seagate/auth/resources/keystore.properties
%config(noreplace) /opt/seagate/auth/resources/static/saml-metadata.xml
%config(noreplace) /opt/seagate/s3/conf/s3config.yaml
%config(noreplace) /opt/seagate/s3/conf/s3server_audit_log.properties
%config(noreplace) /opt/seagate/s3/conf/s3_obj_layout_mapping.yaml
%config(noreplace) /opt/seagate/s3/conf/s3stats-whitelist.yaml
%config(noreplace) /opt/seagate/auth/resources/defaultAclTemplate.xml
%config(noreplace) /opt/seagate/auth/resources/AmazonS3.xsd

%attr(4600, root, root) /opt/seagate/auth/resources/authserver.properties
%attr(4600, root, root) /opt/seagate/auth/resources/authserver-log4j2.xml
%attr(4600, root, root) /opt/seagate/auth/resources/authencryptcli-log4j2.xml
%attr(4600, root, root) /opt/seagate/auth/resources/keystore.properties
%attr(4600, root, root) /opt/seagate/auth/resources/defaultAclTemplate.xml
%attr(4600, root, root) /opt/seagate/auth/resources/AmazonS3.xsd

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
%dir /var/log/seagate/
%dir /var/log/seagate/auth
%dir /var/log/seagate/s3
/etc/cron.hourly/s3logfilerollover.sh
/lib/systemd/system/s3authserver.service
/lib/systemd/system/s3server@.service
/opt/seagate/auth/AuthServer-1.0-0.jar
/opt/seagate/auth/AuthPassEncryptCLI-1.0-0.jar
/opt/seagate/auth/startauth.sh
/opt/seagate/auth/scripts/enc_ldap_passwd_in_cfg.sh
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
/etc/rsyslog.d/rsyslog-tcp-audit.conf

%post
systemctl daemon-reload
systemctl enable s3authserver
systemctl restart rsyslog
