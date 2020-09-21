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


%global pypi_name s3transfer

Name:		python-%{pypi_name}
Version:	0.1.10
Release:	1%{?dist}
Summary:	AWS S3 transfer Manager

License:	ASL 2.0
URL:		https://github.com/boto/s3transfer
Source0:	https://pypi.io/packages/source/s/%{pypi_name}/%{pypi_name}-%{version}.tar.gz
BuildArch:      noarch

%description
A python library to manage Amazon S3 transfers.

%package -n     python2-%{pypi_name}
Summary:        An Amazon S3 Transfer Manager
BuildRequires:  python2-devel
BuildRequires:  python-setuptools
%if %{with tests}
BuildRequires:  python-nose
BuildRequires:  python-mock
BuildRequires:  python-wheel
BuildRequires:  python-futures
BuildRequires:  python2-botocore
BuildRequires:  python-coverage
BuildRequires:  python-unittest2
%endif # tests
Requires:       python-futures
Requires:       python2-botocore
%{?python_provide:%python_provide python2-%{pypi_name}}

%description -n python2-%{pypi_name}
A python library to manage Amazon S3 transfers.

%if 0%{?s3_with_python34:1}
%package -n     python%{python3_pkgversion}-%{pypi_name}
Summary:        An Amazon S3 Transfer Manager
BuildRequires:  python%{python3_pkgversion}-devel
BuildRequires:  python%{python3_pkgversion}-setuptools
%if %{with tests}
BuildRequires:  python%{python3_pkgversion}-nose
BuildRequires:  python%{python3_pkgversion}-mock
BuildRequires:  python%{python3_pkgversion}-wheel
BuildRequires:  python%{python3_pkgversion}-botocore
BuildRequires:  python%{python3_pkgversion}-coverage
BuildRequires:  python%{python3_pkgversion}-unittest2
%endif # tests
Requires:       python%{python3_pkgversion}-botocore
%{?python_provide:%python_provide python%{python3_pkgversion}-%{pypi_name}}

%description -n python%{python3_pkgversion}-%{pypi_name}
A python library to manage Amazon S3 transfers.
%endif # python3

%if 0%{?s3_with_python36:1} || 0%{?s3_with_python36_rhel7:1}
%package -n     python36-%{pypi_name}
Summary:        An Amazon S3 Transfer Manager
BuildRequires:  python3-devel
BuildRequires:  python36-setuptools
%if %{with tests}
BuildRequires:  python36-nose
BuildRequires:  python36-mock
BuildRequires:  python36-wheel
BuildRequires:  python36-botocore
BuildRequires:  python36-coverage
BuildRequires:  python36-unittest2
%endif # tests
Requires:       python36-botocore
%{?python_provide:%python_provide python36-%{pypi_name}}

%description -n python36-%{pypi_name}
A python library to manage Amazon S3 transfers.
%endif # with_python36

%prep
%setup -q -n %{pypi_name}-%{version}
# Remove online tests (see https://github.com/boto/s3transfer/issues/8)
rm -rf tests/integration

%build
%py2_build
%if 0%{?s3_with_python34:1}
%py3_build
%endif # python3
%if 0%{?s3_with_python36:1} || 0%{?s3_with_python36_rhel7:1}
%py3_build
%endif # with_python36

%install
%py2_install
%if 0%{?s3_with_python34:1}
%py3_install
%endif # python3
%if 0%{?s3_with_python36:1} || 0%{?s3_with_python36_rhel7:1}
%py3_install
%endif # with_python36

%if %{with tests}
%check
nosetests-%{python2_version} --with-coverage --cover-erase --cover-package s3transfer --with-xunit --cover-xml -v tests/unit/ tests/functional/
%if 0%{?s3_with_python34:1}
nosetests-%{python%{python3_pkgversion}_version} --with-coverage --cover-erase --cover-package s3transfer --with-xunit --cover-xml -v tests/unit/ tests/functional/
%endif # python3
%if 0%{?s3_with_python36:1} || 0%{?s3_with_python36_rhel7:1}
nosetests-%{python36_version} --with-coverage --cover-erase --cover-package s3transfer --with-xunit --cover-xml -v tests/unit/ tests/functional/
%endif # with_python36
%endif # tests

%files -n python2-%{pypi_name}
%{!?_licensedir:%global license %doc}
%doc README.rst
%license LICENSE.txt
%{python2_sitelib}/%{pypi_name}
%{python2_sitelib}/%{pypi_name}-%{version}-py?.?.egg-info

%if 0%{?s3_with_python34:1}
%files -n python%{python3_pkgversion}-%{pypi_name}
%doc README.rst
%license LICENSE.txt
%{python3_sitelib}/%{pypi_name}
%{python3_sitelib}/%{pypi_name}-%{version}-py?.?.egg-info
%endif # python3

%if 0%{?s3_with_python36:1} || 0%{?s3_with_python36_rhel7:1}
%files -n python36-%{pypi_name}
%doc README.rst
%license LICENSE.txt
%{python3_sitelib}/%{pypi_name}
%{python3_sitelib}/%{pypi_name}-%{version}-py?.?.egg-info
%endif # with_python36

