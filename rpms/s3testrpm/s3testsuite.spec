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

Name:       cortx-s3-test
Version:    %{_s3_test_version}
Release:    %{build_num}_%{_s3_test_git_ver}
Summary:    Seagate S3 Test Suite.

Group:      Development/Tools
License:    Seagate
URL:        https://github.com/Seagate/cortx-s3server
Source0:    %{name}-%{version}-%{_s3_test_git_ver}.tar.gz

BuildRoot:  %{_tmppath}/%{name}-%{version}-%{release}-buildroot
Prefix:     %{_prefix}
BuildArch:  noarch
Vendor:     Seagate

BuildRequires:  python3-rpm-macros
BuildRequires:  python36
BuildRequires:  python3-devel
BuildRequires:  python%{py_short_ver}-setuptools
BuildRequires:  python%{py_short_ver}-wheel

Requires:  python36
Requires:  cortx-s3iamcli
Requires:  s3cmd

%description
Seagate S3 Test Suite

%prep
%setup -n %{name}-%{version}-%{_s3_test_git_ver}

%pre

%build

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/opt/seagate/cortx/s3/scripts/
cp scripts/* $RPM_BUILD_ROOT/opt/seagate/cortx/s3/scripts/
mkdir -p $RPM_BUILD_ROOT/root/
cp .s3cfg $RPM_BUILD_ROOT/root/.s3cfg
mkdir -p $RPM_BUILD_ROOT/etc/ssl/stx-s3-clients/s3/
cp "certs/stx-s3-clients/s3/ca.crt" "$RPM_BUILD_ROOT/etc/ssl/stx-s3-clients/s3/ca.crt"

%clean
rm -rf $RPM_BUILD_ROOT

%files
/opt/seagate/cortx/s3/scripts/s3-sanity-test.sh
/root/.s3cfg
/etc/ssl/stx-s3-clients/s3/ca.crt
%defattr(-,root,root)
