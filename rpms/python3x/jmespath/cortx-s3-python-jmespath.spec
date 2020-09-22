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


%global pypi_name jmespath

Name:		python-%{pypi_name}
Version:	0.9.0
Release:	1%{?dist}
Summary:	JSON Matching Expressions

License:	MIT
URL:		https://github.com/jmespath/jmespath.py
Source0:	https://pypi.python.org/packages/source/j/%{pypi_name}/%{pypi_name}-%{version}.tar.gz
BuildArch:      noarch

BuildRequires:	python2-devel
BuildRequires:  python-setuptools
%if 0%{?s3_with_python34:1}
BuildRequires:  python%{python3_pkgversion}-devel
BuildRequires:  python%{python3_pkgversion}-setuptools
%endif # with_python34
%if 0%{?s3_with_python36:1} || 0%{?s3_with_python36_rhel7:1}
BuildRequires:  python3-devel
BuildRequires:  python36-setuptools
%endif # with_python36

%description
library to extract elements from a JSON document.

%package -n     python2-%{pypi_name}
Summary:        JSON Matching Expressions
%{?python_provide:%python_provide python2-%{pypi_name}}

%description -n python2-%{pypi_name}
library to extract elements from a JSON document.

%if 0%{?s3_with_python34:1}
%package -n     python%{python3_pkgversion}-%{pypi_name}
Summary:        JSON Matching Expressions
%{?python_provide:%python_provide python%{python3_pkgversion}-%{pypi_name}}

%description -n python%{python3_pkgversion}-%{pypi_name}
library to extract elements from a JSON document.
%endif # with_python3

%if 0%{?s3_with_python36:1} || 0%{?s3_with_python36_rhel7:1}
%package -n     python36-%{pypi_name}
Summary:        JSON Matching Expressions
%{?python_provide:%python_provide python36-%{pypi_name}}

%description -n python36-%{pypi_name}
library to extract elements from a JSON document.
%endif # with_python36

%prep
%setup -n %{pypi_name}-%{version}
rm -rf %{pypi_name}.egg-info

%build
%py2_build
%if 0%{?s3_with_python34:1}
%py3_build
%endif # with_python3
%if 0%{?s3_with_python36:1} || 0%{?s3_with_python36_rhel7:1}
%py3_build
%endif # with_python36

%install
%if 0%{?s3_with_python34:1}
%py3_install
cp %{buildroot}/%{_bindir}/jp.py %{buildroot}/%{_bindir}/jp.py-%{python3_version}
%endif # with_python34
%if 0%{?s3_with_python36:1} || 0%{?s3_with_python36_rhel7:1}
%py3_install
cp %{buildroot}/%{_bindir}/jp.py %{buildroot}/%{_bindir}/jp.py-%{python3_version}
%endif # with_python36

%py2_install
cp %{buildroot}/%{_bindir}/jp.py %{buildroot}/%{_bindir}/jp.py-2
ln -sf %{_bindir}/jp.py-2 %{buildroot}/%{_bindir}/jp.py-%{python2_version}


%files
%{!?_licensedir:%global license %doc}
%doc README.rst
%license LICENSE.txt
%{_bindir}/jp.py
%{_bindir}/jp.py-2
%{_bindir}/jp.py-%{python2_version}
%{python2_sitelib}/%{pypi_name}
%{python2_sitelib}/%{pypi_name}-%{version}-py?.?.egg-info

%if 0%{?s3_with_python34:1}
%files -n python%{python3_pkgversion}-%{pypi_name}
%doc README.rst
%license LICENSE.txt
%{_bindir}/jp.py-%{python3_version}
%{python3_sitelib}/%{pypi_name}
%{python3_sitelib}/%{pypi_name}-%{version}-py?.?.egg-info
%endif # with_python34

%if 0%{?s3_with_python36:1} || 0%{?s3_with_python36_rhel7:1}
%files -n python36-%{pypi_name}
%doc README.rst
%license LICENSE.txt
%{_bindir}/jp.py-%{python3_version}
%{python3_sitelib}/%{pypi_name}
%{python3_sitelib}/%{pypi_name}-%{version}-py?.?.egg-info
%endif # with_python36

