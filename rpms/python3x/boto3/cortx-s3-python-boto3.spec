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


%global pypi_name boto3

Name:		python-%{pypi_name}
Version:	1.4.6
Release:	1%{?dist}
Summary:	The AWS SDK for Python

License:	ASL 2.0
URL:		https://github.com/boto/boto3
Source0:	https://pypi.io/packages/source/b/%{pypi_name}/%{pypi_name}-%{version}.tar.gz
BuildArch:      noarch

%description
Boto3 is AWS SDK for python, used to write python applications
to talk to AWS S3 and compatible services.

%if 0%{?s3_with_python34:1}
%package -n     python%{python3_pkgversion}-%{pypi_name}
Summary:        The AWS SDK for Python

BuildRequires:  python%{python3_pkgversion}-devel
BuildRequires:  python%{python3_pkgversion}-setuptools
BuildRequires:  python%{python3_pkgversion}-nose
BuildRequires:  python%{python3_pkgversion}-mock
BuildRequires:  python%{python3_pkgversion}-wheel
BuildRequires:  python%{python3_pkgversion}-botocore
BuildRequires:  python%{python3_pkgversion}-jmespath
BuildRequires:  python%{python3_pkgversion}-s3transfer
Requires:       python%{python3_pkgversion}-botocore >= 1.5.0
Requires:       python%{python3_pkgversion}-jmespath >= 0.7.1
Requires:       python%{python3_pkgversion}-s3transfer >= 0.1.10
%{?python_provide:%python_provide python%{python3_pkgversion}-%{pypi_name}}

%description -n python%{python3_pkgversion}-%{pypi_name}
Boto3 is AWS SDK for python, used to write python applications
to talk to AWS S3 and compatible services.
%endif # with python34

%if 0%{?s3_with_python36:1} || 0%{?s3_with_python36_rhel7:1}
%package -n     python36-%{pypi_name}
Summary:        The AWS SDK for Python

BuildRequires:  python3-devel
BuildRequires:  python36-setuptools
BuildRequires:  python36-nose
BuildRequires:  python36-mock
BuildRequires:  python36-wheel
BuildRequires:  python36-botocore
BuildRequires:  python36-jmespath
BuildRequires:  python36-s3transfer
Requires:       python36-botocore >= 1.5.0
Requires:       python36-jmespath >= 0.7.1
Requires:       python36-s3transfer >= 0.1.10
%{?python_provide:%python_provide python36-%{pypi_name}}

%description -n python36-%{pypi_name}
Boto3 is AWS SDK for python, used to write python applications
to talk to AWS S3 and compatible services.
%endif # with_python36

%prep
%setup -q -n %{pypi_name}-%{version}
rm -rf %{pypi_name}.egg-info
# Remove online tests
rm -rf tests/integration


%build
%if 0%{?s3_with_python34:1}
%py3_build
%endif # with python34
%if 0%{?s3_with_python36:1} || 0%{?s3_with_python36_rhel7:1}
%py3_build
%endif # with_python36


%install
%if 0%{?s3_with_python34:1}
%py3_install
%endif # with python34
%if 0%{?s3_with_python36:1} || 0%{?s3_with_python36_rhel7:1}
%py3_install
%endif # with_python36


%check
%if 0%{?s3_with_python34:1}
%{__python3} setup.py test
%endif # with python34
%if 0%{?s3_with_python36:1} || 0%{?s3_with_python36_rhel7:1}
python3 setup.py test
%endif # with_python36

%if 0%{?s3_with_python34:1}
%files -n python%{python3_pkgversion}-%{pypi_name}
%doc README.rst
%license LICENSE
%{python3_sitelib}/%{pypi_name}
%{python3_sitelib}/%{pypi_name}-%{version}-py?.?.egg-info
%endif # with python34

%if 0%{?s3_with_python36:1} || 0%{?s3_with_python36_rhel7:1}
%files -n python36-%{pypi_name}
%doc README.rst
%license LICENSE
%{python3_sitelib}/%{pypi_name}
%{python3_sitelib}/%{pypi_name}-%{version}-py?.?.egg-info
%endif # with_python36

%changelog
* Sun Aug 13 2017 Fabio Alessandro Locati <fale@fedoraproject.org> 1.4.6-1
- Update to 1.4.6

* Thu Jul 27 2017 Fedora Release Engineering <releng@fedoraproject.org> - 1.4.4-3
- Rebuilt for https://fedoraproject.org/wiki/Fedora_27_Mass_Rebuild

* Sat Feb 11 2017 Fedora Release Engineering <releng@fedoraproject.org> - 1.4.4-2
- Rebuilt for https://fedoraproject.org/wiki/Fedora_26_Mass_Rebuild

* Fri Jan 20 2017 Fabio Alessandro Locati <fale@fedoraproject.org> 1.4.4-1
- Update to 1.4.4

* Wed Dec 28 2016 Fabio Alessandro Locati <fale@fedoraproject.org> 1.4.3-1
- Update to 1.4.3

* Mon Dec 19 2016 Miro Hrončok <mhroncok@redhat.com> - 1.4.2-2
- Rebuild for Python 3.6

* Sat Dec 03 2016 Fabio Alessandro Locati <fale@fedoraproject.org> 1.4.2-1
- Update to 1.4.2

* Mon Oct 10 2016 Fabio Alessandro Locati <fale@fedoraproject.org> - 1.4.1-1
- Update to 1.4.1

* Thu Aug 04 2016 Fabio Alessandro Locati <fale@fedoraproject.org> - 1.4.0-1
- New upstream release

* Tue Jul 19 2016 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 1.3.1-2
- https://fedoraproject.org/wiki/Changes/Automatic_Provides_for_Python_RPM_Packages

* Sat May 28 2016 Fabio Alessandro Locati <fale@fedoraproject.org> - 1.3.1-1
- New upstream release

* Tue Mar 29 2016 Fabio Alessandro Locati <fale@fedoraproject.org> - 1.3.0-1
- New upstream release

* Fri Feb 19 2016 Fabio Alessandro Locati <fale@fedoraproject.org> - 1.2.4-1
- New upstream release

* Thu Feb 11 2016 Fabio Alessandro Locati <fale@fedoraproject.org> - 1.2.3-3
- Fix python2- subpackage to require python-future

* Thu Feb 04 2016 Fedora Release Engineering <releng@fedoraproject.org> - 1.2.3-2
- Rebuilt for https://fedoraproject.org/wiki/Fedora_24_Mass_Rebuild

* Tue Dec 29 2015 Fabio Alessandro Locati <fale@fedoraproject.org> - 1.2.3-1
- Initial package.

