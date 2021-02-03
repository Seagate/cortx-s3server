#
# Copyright (c) 2020 Seagate Technology LLC and/or its Affiliates
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# For any questions about this software or licensing,
# please email opensource@seagate.com or cortx-questions@seagate.com.
#

%if 0%{?disable_cortxmotr_dependencies:1}
%bcond_with cortx_motr
%else
%bcond_without cortx_motr
%endif

# cortx-motr version
%define h_cortxmotr_version %(rpm -q --queryformat '%{VERSION}-%{RELEASE}' cortx-motr)

# build number
%define build_num  %( test -n "$build_number" && echo "$build_number" || echo 1 )

%global py_ver 3.6

%if 0%{?el7}
# pybasever without the dot:
%global py_short_ver 36
%endif

%if 0%{?el8}
# pybasever without the dot:
%global py_short_ver 3
%endif

# XXX For strange reason setup.py uses /usr/lib
# but %{_libdir} resolves to /usr/lib64 with python3.6
#%global py36_sitelib %{_libdir}/python%{py_ver}
%global py36_sitelib /usr/lib/python%{py_ver}/site-packages

Name:       cortx-s3server
Version:    %{_s3_version}
Release:    %{build_num}_%{_s3_git_ver}_%{?dist:el7}
Summary:    CORTX s3server

Group:      Development/Tools
License:    Seagate
URL:        https://github.com/Seagate/cortx-s3server
Source:     %{name}-%{version}-%{_s3_git_ver}.tar.gz

BuildRequires: automake
BuildRequires: bazel
BuildRequires: cmake >= 2.8.12
BuildRequires: libtool
%if %{with cortx_motr}
BuildRequires: cortx-motr cortx-motr-devel
%endif
BuildRequires: openssl openssl-devel
BuildRequires: java-1.8.0-openjdk
BuildRequires: java-1.8.0-openjdk-devel
BuildRequires: maven
BuildRequires: unzip clang zlib-devel
BuildRequires: libxml2 libxml2-devel
BuildRequires: libyaml libyaml-devel
BuildRequires: yaml-cpp yaml-cpp-devel
BuildRequires: gflags gflags-devel
BuildRequires: glog glog-devel
BuildRequires: gtest
BuildRequires: gtest-devel
BuildRequires: git git-clang-format
BuildRequires: log4cxx_cortx log4cxx_cortx-devel
BuildRequires: hiredis hiredis-devel
# Required by S3 background delete based on python
BuildRequires: python3-rpm-macros
BuildRequires: python36
BuildRequires: python%{py_short_ver}-devel
BuildRequires: python%{py_short_ver}-setuptools
BuildRequires: python%{py_short_ver}-dateutil
BuildRequires: python%{py_short_ver}-yaml
BuildRequires: python%{py_short_ver}-pika
%if 0%{?el7}
BuildRequires: python-keyring python-futures
%endif
# TODO for rhel 8

%if %{with cortx_motr}
# Uncomment below line to enable motr version check during s3server rpm installation
#Requires: cortx-motr = %{h_cortxmotr_version}
Requires: cortx-motr
%endif
Requires: libxml2
Requires: libyaml
#Supported openssl versions -- CentOS 7 its 1.0.2k, RHEL8 its 1.1.1
Requires: openssl
Requires: yaml-cpp
Requires: gflags
Requires: glog
Requires: pkgconfig
Requires: log4cxx_cortx log4cxx_cortx-devel
# Required by S3 background delete based on python
Requires: python36
Requires: python%{py_short_ver}-yaml
Requires: python%{py_short_ver}-pika
%if 0%{?el7}
Requires: python-keyring python-futures
%endif

# Java used by Auth server
Requires: java-1.8.0-openjdk-headless
Requires: PyYAML
Requires: hiredis

%description
S3 server provides S3 REST API interface support for Motr object storage.

%prep
%setup -n %{name}-%{version}-%{_s3_git_ver}

%pre
if [ -f /opt/seagate/cortx/s3/conf/s3config.yaml.sample ]; then
cp -f /opt/seagate/cortx/s3/conf/s3config.yaml.sample /tmp/s3config.yaml.sample.old
fi
if [ -f /opt/seagate/cortx/s3/s3backgrounddelete/config.yaml.sample ]; then
    cp -f /opt/seagate/cortx/s3/s3backgrounddelete/config.yaml.sample /tmp/config.yaml.sample.old
fi
if [ -f /opt/seagate/cortx/auth/resources/keystore.properties.sample ]; then
    cp -f /opt/seagate/cortx/auth/resources/keystore.properties.sample /tmp/keystore.properties.sample.old
fi
if [ -f /opt/seagate/cortx/auth/resources/authserver.properties.sample ]; then
    cp -f /opt/seagate/cortx/auth/resources/authserver.properties.sample /tmp/authserver.properties.sample.old
fi

%build
%if %{with cortx_motr}
./rebuildall.sh --no-check-code --no-install --no-s3ut-build --no-s3mempoolut-build --no-s3mempoolmgrut-build --no-java-tests
%else
./rebuildall.sh --no-check-code --no-install --no-s3ut-build --no-s3mempoolut-build --no-s3mempoolmgrut-build --no-java-tests --no-motr-rpm --use-build-cache
%endif

# Build the background delete python module.
mkdir -p %{_builddir}/%{name}-%{version}-%{_s3_git_ver}/s3backgrounddelete/build/lib/s3backgrounddelete
cd s3backgrounddelete/s3backgrounddelete
python%{py_ver} -m compileall -b *.py
cp  *.pyc %{_builddir}/%{name}-%{version}-%{_s3_git_ver}/s3backgrounddelete/build/lib/s3backgrounddelete

# Build s3cortxutils/s3confstore python module
mkdir -p %{_builddir}/%{name}-%{version}-%{_s3_git_ver}/s3cortxutils/s3confstore/build/lib/s3confstore
cd %{_builddir}/%{name}-%{version}-%{_s3_git_ver}/s3cortxutils/s3confstore/s3confstore
python%{py_ver} -m compileall -b *.py
cp *.pyc %{_builddir}/%{name}-%{version}-%{_s3_git_ver}/s3cortxutils/s3confstore/build/lib/s3confstore

# Build scripts/s3haproxyconfig python module
mkdir -p %{_builddir}/%{name}-%{version}-%{_s3_git_ver}/scripts/haproxy/s3haproxyconfig/build/lib/s3haproxyconfig
cd %{_builddir}/%{name}-%{version}-%{_s3_git_ver}/scripts/haproxy/s3haproxyconfig/s3haproxyconfig
python%{py_ver} -m compileall -b *.py
cp *.pyc %{_builddir}/%{name}-%{version}-%{_s3_git_ver}/scripts/haproxy/s3haproxyconfig/build/lib/s3haproxyconfig

# Build s3setup python module
mkdir -p %{_builddir}/%{name}-%{version}-%{_s3_git_ver}/scripts/provisioning/s3setup/build/lib/s3setup
cd %{_builddir}/%{name}-%{version}-%{_s3_git_ver}/scripts/provisioning/s3setup/s3setup
python%{py_ver} -m compileall -b *.py
cp *.pyc %{_builddir}/%{name}-%{version}-%{_s3_git_ver}/scripts/provisioning/s3setup/build/lib/s3setup

# Build the s3cortxutils/s3MsgBus wrapper python module
mkdir -p %{_builddir}/%{name}-%{version}-%{_s3_git_ver}/s3cortxutils/s3msgbus/build/lib/s3msgbus
cd %{_builddir}/%{name}-%{version}-%{_s3_git_ver}/s3cortxutils/s3msgbus/s3msgbus
python%{py_ver} -m compileall -b *.py
cp  *.pyc %{_builddir}/%{name}-%{version}-%{_s3_git_ver}/s3cortxutils/s3msgbus/build/lib/s3msgbus 

# Build the s3cortxutils/s3cipher wrapper python module
mkdir -p %{_builddir}/%{name}-%{version}-%{_s3_git_ver}/s3cortxutils/s3cipher/build/lib/s3cipher
cd %{_builddir}/%{name}-%{version}-%{_s3_git_ver}/s3cortxutils/s3cipher/s3cipher
python%{py_ver} -m compileall -b *.py
cp  *.pyc %{_builddir}/%{name}-%{version}-%{_s3_git_ver}/s3cortxutils/s3cipher/build/lib/s3cipher 

echo "build complete"


%install
rm -rf %{buildroot}
./installhelper.sh %{buildroot} --release

# Install the background delete python module.
cd %{_builddir}/%{name}-%{version}-%{_s3_git_ver}/s3backgrounddelete
python%{py_ver} setup.py install --single-version-externally-managed -O1 --root=$RPM_BUILD_ROOT --version=%{version}

# Install the s3msg bus wrapper module
cd %{_builddir}/%{name}-%{version}-%{_s3_git_ver}/s3cortxutils/s3msgbus
python%{py_ver} setup.py install --single-version-externally-managed -O1 --root=$RPM_BUILD_ROOT --version=%{version}

# Install the s3msg bus wrapper module
cd %{_builddir}/%{name}-%{version}-%{_s3_git_ver}/s3cortxutils/s3cipher
python%{py_ver} setup.py install --single-version-externally-managed -O1 --root=$RPM_BUILD_ROOT --version=%{version}

# Install s3confstore python module
cd %{_builddir}/%{name}-%{version}-%{_s3_git_ver}/s3cortxutils/s3confstore
python%{py_ver} setup.py install --single-version-externally-managed -O1 --root=$RPM_BUILD_ROOT --version=%{version}

# Install s3haproxyconfig python module
cd %{_builddir}/%{name}-%{version}-%{_s3_git_ver}/scripts/haproxy/s3haproxyconfig
python%{py_ver} setup.py install --single-version-externally-managed -O1 --root=$RPM_BUILD_ROOT --version=%{version}

# Install s3setup python module
cd %{_builddir}/%{name}-%{version}-%{_s3_git_ver}/scripts/provisioning/s3setup
python%{py_ver} setup.py install --single-version-externally-managed -O1 --root=$RPM_BUILD_ROOT --version=%{version}

%clean
bazel clean
cd auth
./mvnbuild.sh clean
cd ..
rm -rf %{buildroot}

%files
%defattr(-,root,root,-)
# config file doesnt get replaced during rpm update if changed
%config /opt/seagate/cortx/auth/resources/authserver.properties.sample
%config(noreplace) /opt/seagate/cortx/auth/resources/authserver-log4j2.xml
%config(noreplace) /opt/seagate/cortx/auth/resources/authencryptcli-log4j2.xml
%config /opt/seagate/cortx/auth/resources/keystore.properties.sample
%config(noreplace) /opt/seagate/cortx/auth/resources/static/saml-metadata.xml
%config(noreplace) /opt/seagate/cortx/auth/resources/s3authserver.jks
%config /opt/seagate/cortx/s3/conf/s3config.yaml.sample
%config(noreplace) /opt/seagate/cortx/s3/conf/s3server_audit_log.properties
%config(noreplace) /opt/seagate/cortx/s3/conf/s3_obj_layout_mapping.yaml
%config(noreplace) /opt/seagate/cortx/s3/conf/s3stats-allowlist.yaml
%config(noreplace) /opt/seagate/cortx/auth/resources/defaultAclTemplate.xml
%config(noreplace) /opt/seagate/cortx/auth/resources/AmazonS3.xsd
%config /opt/seagate/cortx/s3/s3backgrounddelete/config.yaml.sample
%config(noreplace) /opt/seagate/cortx/s3/s3backgrounddelete/s3_cluster.yaml

%attr(4600, root, root) /opt/seagate/cortx/auth/resources/authserver.properties.sample
%attr(4600, root, root) /opt/seagate/cortx/auth/resources/authserver-log4j2.xml
%attr(4600, root, root) /opt/seagate/cortx/auth/resources/authencryptcli-log4j2.xml
%attr(4600, root, root) /opt/seagate/cortx/auth/resources/keystore.properties.sample
%attr(4600, root, root) /opt/seagate/cortx/auth/resources/defaultAclTemplate.xml
%attr(4600, root, root) /opt/seagate/cortx/auth/resources/AmazonS3.xsd
%attr(4600, root, root) /opt/seagate/cortx/auth/resources/s3authserver.jks
%attr(4600, root, root) /opt/seagate/cortx/s3/conf/s3config.yaml.sample 
%attr(4600, root, root) /opt/seagate/cortx/s3/s3backgrounddelete/config.yaml.sample

%dir /opt/seagate/cortx/
%dir /opt/seagate/cortx/auth
%dir /opt/seagate/cortx/auth/resources
%dir /opt/seagate/cortx/auth/resources/static
%dir /opt/seagate/cortx/s3
%dir /opt/seagate/cortx/s3/addb-plugin
%dir /opt/seagate/cortx/s3/bin
%dir /opt/seagate/cortx/s3/conf
%dir /opt/seagate/cortx/s3/libevent
%dir /opt/seagate/cortx/s3/libevent/pkgconfig
%dir /opt/seagate/cortx/s3/nodejs
%dir /opt/seagate/cortx/s3/resources
%dir /opt/seagate/cortx/s3/install/haproxy
%dir /var/log/seagate/
%dir /var/log/seagate/auth
%dir /var/log/seagate/s3
/etc/cron.hourly/s3logfilerollover.sh
/etc/cron.hourly/s3m0tracelogfilerollover.sh
/etc/cron.hourly/s3addblogfilerollover.sh
/lib/systemd/system/s3authserver.service
/lib/systemd/system/s3server@.service
/lib/systemd/system/s3backgroundproducer.service
/lib/systemd/system/s3backgroundconsumer.service
/opt/seagate/cortx/auth/AuthServer-1.0-0.jar
/opt/seagate/cortx/auth/AuthPassEncryptCLI-1.0-0.jar
/opt/seagate/cortx/auth/startauth.sh
/opt/seagate/cortx/auth/scripts/swupdate/merge.sh
/opt/seagate/cortx/auth/scripts/enc_ldap_passwd_in_cfg.sh
/opt/seagate/cortx/auth/scripts/change_ldap_passwd.ldif
/opt/seagate/cortx/auth/scripts/s3authserver.jks_template
/opt/seagate/cortx/auth/scripts/create_auth_jks_password.sh
/opt/seagate/cortx/auth/resources/s3authserver.jks
/opt/seagate/cortx/s3/scripts/s3-sanity-test.sh
/opt/seagate/cortx/s3/scripts/s3_bundle_generate.sh
/opt/seagate/cortx/s3/docs/openldap_backup_readme
/opt/seagate/cortx/s3/docs/s3_log_rotation_guide.txt
/opt/seagate/cortx/s3/addb-plugin/libs3addbplugin.so
/opt/seagate/cortx/s3/bin/motrkvscli
/opt/seagate/cortx/s3/bin/s3server
/opt/seagate/cortx/s3/libevent/libevent-2.1.so.6
/opt/seagate/cortx/s3/libevent/libevent-2.1.so.6.0.4
/opt/seagate/cortx/s3/libevent/libevent.a
/opt/seagate/cortx/s3/libevent/libevent.la
/opt/seagate/cortx/s3/libevent/libevent.so
/opt/seagate/cortx/s3/libevent/libevent_core-2.1.so.6
/opt/seagate/cortx/s3/libevent/libevent_core-2.1.so.6.0.4
/opt/seagate/cortx/s3/libevent/libevent_core.a
/opt/seagate/cortx/s3/libevent/libevent_core.la
/opt/seagate/cortx/s3/libevent/libevent_core.so
/opt/seagate/cortx/s3/libevent/libevent_extra-2.1.so.6
/opt/seagate/cortx/s3/libevent/libevent_extra-2.1.so.6.0.4
/opt/seagate/cortx/s3/libevent/libevent_extra.a
/opt/seagate/cortx/s3/libevent/libevent_extra.la
/opt/seagate/cortx/s3/libevent/libevent_extra.so
/opt/seagate/cortx/s3/libevent/libevent_openssl-2.1.so.6
/opt/seagate/cortx/s3/libevent/libevent_openssl-2.1.so.6.0.4
/opt/seagate/cortx/s3/libevent/libevent_openssl.a
/opt/seagate/cortx/s3/libevent/libevent_openssl.la
/opt/seagate/cortx/s3/libevent/libevent_openssl.so
/opt/seagate/cortx/s3/libevent/libevent_pthreads-2.1.so.6
/opt/seagate/cortx/s3/libevent/libevent_pthreads-2.1.so.6.0.4
/opt/seagate/cortx/s3/libevent/libevent_pthreads.a
/opt/seagate/cortx/s3/libevent/libevent_pthreads.la
/opt/seagate/cortx/s3/libevent/libevent_pthreads.so
/opt/seagate/cortx/s3/libevent/pkgconfig/libevent.pc
/opt/seagate/cortx/s3/libevent/pkgconfig/libevent_openssl.pc
/opt/seagate/cortx/s3/libevent/pkgconfig/libevent_pthreads.pc
/opt/seagate/cortx/s3/libevent/pkgconfig/libevent_core.pc
/opt/seagate/cortx/s3/libevent/pkgconfig/libevent_extra.pc
/opt/seagate/cortx/s3/install/haproxy/503.http
/opt/seagate/cortx/s3/install/haproxy/haproxy_osver7.cfg
/opt/seagate/cortx/s3/install/haproxy/haproxy_osver8.cfg
/opt/seagate/cortx/s3/install/haproxy/logrotate/haproxy
/opt/seagate/cortx/s3/install/haproxy/logrotate/logrotate
/opt/seagate/cortx/s3/install/haproxy/rsyslog.d/haproxy.conf
/opt/seagate/cortx/s3/install/haproxy/ssl/s3.seagate.com.crt
/opt/seagate/cortx/s3/install/haproxy/ssl/s3.seagate.com.pem
/opt/seagate/cortx/s3/install/haproxy/503.http
/opt/seagate/cortx/s3/install/ldap/syncprov_mod.ldif
/opt/seagate/cortx/s3/install/ldap/syncprov.ldif
/opt/seagate/cortx/s3/install/ldap/replicate.ldif
/opt/seagate/cortx/s3/install/ldap/resultssizelimit.ldif
/opt/seagate/cortx/s3/install/ldap/check_ldap_replication.sh
/opt/seagate/cortx/s3/install/ldap/test_data.ldif
/opt/seagate/cortx/s3/install/ldap/run_check_ldap_replication_in_loop.sh
/opt/seagate/cortx/s3/install/ldap/create_replication_account.ldif
/opt/seagate/cortx/s3/install/ldap/replication/syncprov_mod.ldif
/opt/seagate/cortx/s3/install/ldap/replication/olcserverid.ldif
/opt/seagate/cortx/s3/install/ldap/replication/syncprov.ldif
/opt/seagate/cortx/s3/install/ldap/replication/data.ldif
/opt/seagate/cortx/s3/install/ldap/replication/config.ldif
/opt/seagate/cortx/s3/install/ldap/replication/syncprov_config.ldif
/opt/seagate/cortx/s3/install/ldap/replication/deltaReplication.ldif
/opt/seagate/cortx/s3/install/ldap/replication/setupReplicationScript.sh
/opt/seagate/cortx/s3/install/ldap/replication/serverIdTemplate.ldif
/opt/seagate/cortx/s3/install/ldap/replication/configTemplate.ldif
/opt/seagate/cortx/s3/install/ldap/replication/dataTemplate.ldif
/opt/seagate/cortx/s3/install/ldap/slapdlog.ldif
/opt/seagate/cortx/s3/install/ldap/s3slapdindex.ldif
/opt/seagate/cortx/s3/install/ldap/rsyslog.d/slapdlog.conf
/opt/seagate/cortx/s3/install/ldap/background_delete_account.ldif
/opt/seagate/cortx/s3/install/ldap/create_background_delete_account.sh
/opt/seagate/cortx/s3/install/ldap/delete_background_delete_account.sh
/opt/seagate/cortx/s3/install/ldap/cfg_ldap.ldif
/opt/seagate/cortx/s3/install/ldap/cn={1}s3user.ldif
/opt/seagate/cortx/s3/install/ldap/iam-admin-access.ldif
/opt/seagate/cortx/s3/install/ldap/iam-admin.ldif
/opt/seagate/cortx/s3/install/ldap/iam-constraints.ldif
/opt/seagate/cortx/s3/install/ldap/ldap-init.ldif
/opt/seagate/cortx/s3/install/ldap/olcDatabase={2}mdb.ldif
/opt/seagate/cortx/s3/install/ldap/ppolicy-default.ldif
/opt/seagate/cortx/s3/install/ldap/ppolicymodule.ldif
/opt/seagate/cortx/s3/install/ldap/ppolicyoverlay.ldif
/opt/seagate/cortx/s3/install/ldap/setup_ldap.sh
/opt/seagate/cortx/s3/resources/s3_error_messages.json
/opt/seagate/cortx/s3/s3startsystem.sh
/opt/seagate/cortx/s3/s3stopsystem.sh
/opt/seagate/cortx/s3/start-s3-iopath-services.sh
/opt/seagate/cortx/s3/stop-s3-iopath-services.sh
/opt/seagate/cortx/s3/reset/precheck.py
/opt/seagate/cortx/s3/reset/reset_s3.sh
/opt/seagate/cortx/s3/reset/clean_open_ldap_by_s3.sh
/opt/seagate/cortx/s3/conf/setup.yaml
/opt/seagate/cortx/auth/resources/authserver_unsafe_attributes.yaml
/opt/seagate/cortx/auth/resources/keystore_unsafe_attributes.yaml
/opt/seagate/cortx/s3/conf/s3config_unsafe_attributes.yaml
/opt/seagate/cortx/s3/s3backgrounddelete/s3backgrounddelete_unsafe_attributes.yaml
/opt/seagate/cortx/s3/mini-prov/s3setup_prereqs.json
%attr(755, root, root) /opt/seagate/cortx/s3/bin/s3_setup
%attr(755, root, root) /opt/seagate/cortx/s3/s3backgrounddelete/s3backgroundconsumer
%attr(755, root, root) /opt/seagate/cortx/s3/s3backgrounddelete/s3backgroundproducer
/etc/rsyslog.d/rsyslog-tcp-audit.conf
/etc/rsyslog.d/elasticsearch.conf
/etc/keepalived/keepalived.conf.main
/etc/logrotate.d/s3auditlog
/etc/logrotate.d/openldap
%{_bindir}/s3backgroundconsumer
%{_bindir}/s3backgroundproducer
%{_bindir}/s3cipher
%{_bindir}/s3msgbus
%{_bindir}/s3confstore
%{_bindir}/s3haproxyconfig
%{_bindir}/s3setup
%{py36_sitelib}/s3backgrounddelete/config/*.yaml
%{py36_sitelib}/s3backgrounddelete/config/s3_background_delete_config.yaml.sample
%{py36_sitelib}/s3backgrounddelete/*.pyc
%{py36_sitelib}/s3backgrounddelete-%{version}-py?.?.egg-info
%{py36_sitelib}/s3msgbus/*.pyc
%{py36_sitelib}/s3msgbus-%{version}-py?.?.egg-info
%{py36_sitelib}/s3cipher/*.pyc
%{py36_sitelib}/s3cipher-%{version}-py?.?.egg-info
%{py36_sitelib}/s3confstore/*.pyc
%{py36_sitelib}/s3confstore-%{version}-py?.?.egg-info
%{py36_sitelib}/s3haproxyconfig/*.pyc
%{py36_sitelib}/s3haproxyconfig-%{version}-py?.?.egg-info
%{py36_sitelib}/s3setup/*.pyc
%{py36_sitelib}/s3setup-%{version}-py?.?.egg-info
%exclude %{py36_sitelib}/s3backgrounddelete/__pycache__/*
%exclude %{py36_sitelib}/s3backgrounddelete/*.py
%exclude %{py36_sitelib}/s3confstore/*.py
%exclude %{py36_sitelib}/s3confstore/__pycache__/*
%exclude %{py36_sitelib}/s3haproxyconfig/*.py
%exclude %{py36_sitelib}/s3haproxyconfig/__pycache__/*
%exclude %{py36_sitelib}/s3setup/*.py
%exclude %{py36_sitelib}/s3setup/__pycache__/*
%exclude %{py36_sitelib}/s3backgrounddelete/s3backgroundconsumer
%exclude %{py36_sitelib}/s3msgbus/s3msgbus
%exclude %{py36_sitelib}/s3msgbus/__pycache__/*
%exclude %{py36_sitelib}/s3msgbus/*.py
%exclude %{py36_sitelib}/s3cipher/s3cipher
%exclude %{py36_sitelib}/s3cipher/__pycache__/*
%exclude %{py36_sitelib}/s3cipher/*.py
%exclude %{py36_sitelib}/s3confstore/s3confstore
%exclude %{py36_sitelib}/s3haproxyconfig/s3haproxyconfig
%exclude %{py36_sitelib}/s3setup/s3setup
%exclude %{py36_sitelib}/s3backgrounddelete/s3backgroundproducer
%exclude /opt/seagate/cortx/s3/reset/precheck.pyc
%exclude /opt/seagate/cortx/s3/reset/precheck.pyo
%exclude /opt/seagate/cortx/s3/install/haproxy/s3haproxyconfig

%post
sh /opt/seagate/cortx/auth/scripts/swupdate/merge.sh
systemctl daemon-reload
systemctl enable s3authserver
systemctl restart rsyslog
openssl_version=`rpm -q --queryformat '%{VERSION}' openssl`
if [ "$openssl_version" != "1.0.2k" ] && [ "$openssl_version" != "1.1.1" ]; then
  echo "Warning: Unsupported (untested) openssl version [$openssl_version] is installed which may work."
  echo "Supported openssl versions are [1.0.2k, 1.1.1]"
fi
