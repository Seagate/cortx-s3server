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

Name:		ossperf
Version:	3.0
Release:	1
Summary:	ossperf tool for test

License:	GPL
URL:		https://github.com/christianbaun/ossperf
Source0:	%{name}-%{version}.tar.gz
Patch:          ossperf.patch
Requires:	s3cmd >= 1.6.1
Requires:       parallel
Requires:       bc

%description -n ossperf
ossperf analyzes performance and data integrity of s3 compatible storage solutions.

%prep
%setup -q
%patch -p1

%install
rm -rf %{buildroot}
install -d $RPM_BUILD_ROOT%{_bindir}/
cp  ossperf.sh $RPM_BUILD_ROOT%{_bindir}/

%clean
rm -rf %{buildroot}

%files -n ossperf
%license LICENSE
%doc README.md
%{_bindir}/ossperf.sh

