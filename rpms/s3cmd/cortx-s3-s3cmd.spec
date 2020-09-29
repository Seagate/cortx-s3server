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

%if 0%{?s3_with_python36:1}
%global py_ver 36
%endif
%if 0%{?s3_with_python36_ver8:1}
%global py_ver 3
%endif

%global python_sitearch %python3_sitearch
%global python_sitelib %python3_sitelib
%global __python %__python3
%global py_package_prefix python%{python3_pkgversion}

%{!?python_sitelib: %define python_sitelib %(%{__python} -c "from distutils.sysconfig import get_python_lib; print get_python_lib()")}

%global commit 4c2489361d353db1a1815172a6143c8f5a2d1c40
%global shortcommit 4c24893

Name:		s3cmd
Version:	1.6.1
Release:	1%{?dist}
Summary:	Tool for accessing Amazon Simple Storage Service

Group:		Applications/Internet
License:	GPLv2
URL:		https://github.com/Seagate/s3cmd
Source0:	%{name}-%{version}-%{shortcommit}.tar.gz
Patch0:         s3cmd_%{version}_max_retries.patch
BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)
BuildArch:      noarch

BuildRequires:	python36-devel

%if 0%{?s3_with_python36_ver8:1}
BuildRequires:  python3-dateutil
BuildRequires:  python3-setuptools
Requires:	python3-magic
%endif
%if 0%{?s3_with_python36:1}
BuildRequires:  python36-dateutil
BuildRequires:  python36-setuptools
Requires:       python-magic
%endif

%description
s3cmd is a CLI based s3-client.

%prep
%setup -q -n %{name}-%{version}-%{shortcommit}
%patch0 -p1


%install
rm -rf $RPM_BUILD_ROOT
S3CMD_PACKAGING=Yes python setup.py install --prefix=%{_prefix} --root=$RPM_BUILD_ROOT
install -d $RPM_BUILD_ROOT%{_mandir}/man1
install -m 644 s3cmd.1 $RPM_BUILD_ROOT%{_mandir}/man1



%files
%defattr(-,root,root,-)
%{_bindir}/s3cmd
%{_mandir}/man1/s3cmd.1*
%{python2_sitelib}/S3
%{python2_sitelib}/s3cmd*.egg-info
%doc NEWS README.md

