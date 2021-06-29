#
# Copyright (c) 2021 Seagate Technology LLC and/or its Affiliates
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

Name:       cortx-s3-test
Version:    %{_s3_test_version}
Release:    %{build_num}_%{_s3_test_git_ver}
Summary:    Seagate S3 Test Suite. It requires s3iamcli, s3cmd and s3server to be present/installed

Group:      Development/Tools
License:    Seagate
URL:        https://github.com/Seagate/cortx-s3server
Source:    %{name}-%{version}-%{_s3_test_git_ver}.tar.gz

BuildRoot:  %{_tmppath}/%{name}-%{version}-%{release}-buildroot
Prefix:     %{_prefix}
BuildArch:  noarch
Vendor:     Seagate

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
cp scripts/s3-sanity/s3-sanity-test.sh $RPM_BUILD_ROOT/opt/seagate/cortx/s3/scripts/s3-sanity-test.sh
mkdir -p $RPM_BUILD_ROOT/root/
cp .s3cfg $RPM_BUILD_ROOT/root/.s3cfg
mkdir -p $RPM_BUILD_ROOT/etc/ssl/stx-s3-clients/s3/
cp "ansible/files/certs/stx-s3-clients/s3/ca.crt" "$RPM_BUILD_ROOT/etc/ssl/stx-s3-clients/s3/ca.crt"

%clean
rm -rf $RPM_BUILD_ROOT

%files
/opt/seagate/cortx/s3/scripts/s3-sanity-test.sh
/root/.s3cfg
/etc/ssl/stx-s3-clients/s3/ca.crt
%defattr(-,root,root)
