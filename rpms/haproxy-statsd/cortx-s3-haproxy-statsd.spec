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

Name:		haproxy-statsd
Version:	1.0
Release:	1
Summary:	tool to send haproxy statistics to statsd

License:	MIT
URL:		https://github.com/softlayer/haproxy-statsd
Source:		%{name}-%{version}.tar.gz
Patch:          haproxy-statsd.patch
Requires:	haproxy, python-psutil

%description -n haproxy-statsd
This script sends statistics from haproxy to statsd server.

%prep
%setup -q
%patch -p1

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}/opt/seagate/cortx/s3-haproxy-statsd/
cp haproxy-statsd.* %{buildroot}/opt/seagate/cortx/s3-haproxy-statsd/
cp LICENSE  %{buildroot}/opt/seagate/cortx/s3-haproxy-statsd/
cp README.md  %{buildroot}/opt/seagate/cortx/s3-haproxy-statsd/

%clean
rm -rf %{buildroot}

%files
%defattr(-,root,root,-)
%dir /opt/seagate/cortx/s3-haproxy-statsd/
%attr(0755, root, root) /opt/seagate/cortx/s3-haproxy-statsd/haproxy-statsd.py
/opt/seagate/cortx/s3-haproxy-statsd/haproxy-statsd.conf
/opt/seagate/cortx/s3-haproxy-statsd/LICENSE
/opt/seagate/cortx/s3-haproxy-statsd/README.md

%exclude /opt/seagate/cortx/s3-haproxy-statsd/*.pyc
%exclude /opt/seagate/cortx/s3-haproxy-statsd/*.pyo

%license LICENSE
%doc README.md

