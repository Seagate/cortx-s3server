# Copied from https://github.com/softlayer/haproxy-statsd
#Unless otherwise noted, all files are released under the MIT license,
#exceptions contain licensing information in them.
# 
#  Copyright (C) 2013 SoftLayer Technologies, Inc.
# 
#Permission is hereby granted, free of charge, to any person obtaining a copy
#of this software and associated documentation files (the "Software"), to deal
#in the Software without restriction, including without limitation the rights
#to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
#copies of the Software, and to permit persons to whom the Software is
#furnished to do so, subject to the following conditions:
# 
#The above copyright notice and this permission notice shall be included in
#all copies or substantial portions of the Software.
# 
#THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
#IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
#AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
#LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
#OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
#SOFTWARE.
Name:           haproxy-statsd
Version:        1.0
Release:        1
Summary:        tool to send haproxy statistics to statsd

License:        MIT
URL:            https://github.com/softlayer/haproxy-statsd
Source:         %{name}-%{version}.tar.gz
Patch:          haproxy-statsd.patch
Requires:       haproxy, python-psutil

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
