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

%global pypi_name wheel

Name:		python-%{pypi_name}
Version:	0.24.0
Release:	2%{?dist}
Summary:	A built-package format for Python

License:	MIT
URL:		http://bitbucket.org/dholth/wheel
Source0:	https://pypi.python.org/packages/source/w/%{pypi_name}/%{pypi_name}-%{version}.tar.gz
BuildArch:      noarch

BuildRequires:  python-devel
BuildRequires:  python-setuptools

BuildRequires:  python-jsonschema
BuildRequires:  python-keyring

%if 0%{?s3_with_python34:1}
BuildRequires:  python%{python3_pkgversion}-devel
BuildRequires:  python%{python3_pkgversion}-setuptools
%endif # if with_python34

%if 0%{?s3_with_python36:1} || 0%{?s3_with_python36_rhel7:1}
BuildRequires:  python3-devel
BuildRequires:  python36-setuptools
%endif # if with_python36

%description
A built-package format for Python.
A wheel is a ZIP-format archive with a specially formatted filename and the
.whl extension. It is designed to contain all the files for a PEP 376
compatible install in a way that is very close to the on-disk format.

%if 0%{?s3_with_python34:1}
%package -n     python%{python3_pkgversion}-%{pypi_name}
Summary:        A built-package format for Python

%description -n python%{python3_pkgversion}-%{pypi_name}
A built-package format for Python.

A wheel is a ZIP-format archive with a specially formatted filename and the
.whl extension. It is designed to contain all the files for a PEP 376
compatible install in a way that is very close to the on-disk format.

This is package contains Python 3 version of the package.

%endif # with_python3

%if 0%{?s3_with_python36:1} || 0%{?s3_with_python36_rhel7:1}
%package -n     python36-%{pypi_name}
Summary:        A built-package format for Python

%description -n python36-%{pypi_name}
A built-package format for Python.

A wheel is a ZIP-format archive with a specially formatted filename and the
.whl extension. It is designed to contain all the files for a PEP 376
compatible install in a way that is very close to the on-disk format.

This is package contains Python 3 version of the package.
%endif # with_python36


%prep
%setup -q -n %{pypi_name}-%{version}

# remove unneeded shebangs
sed -ie '1d' %{pypi_name}/{egg2wheel,wininst2wheel}.py

%if 0%{?s3_with_python34:1}
rm -rf %{py3dir}
cp -a . %{py3dir}
%endif # with_python3

%if 0%{?s3_with_python36:1} || 0%{?s3_with_python36_rhel7:1}
rm -rf %{py3dir}-for36
cp -a . %{py3dir}-for36
%endif # with_python36


%build
%{__python} setup.py build

%if 0%{?s3_with_python34:1}
pushd %{py3dir}
%{__python3} setup.py build
popd
%endif # with_python3

%if 0%{?s3_with_python36:1} || 0%{?s3_with_python36_rhel7:1}
pushd %{py3dir}-for36
python3 setup.py build
popd
%endif # with_python36


%install
# Must do the subpackages' install first because the scripts in /usr/bin are
# overwritten with every setup.py install (and we want the python2 version
# to be the default for now).
%if 0%{?s3_with_python34:1}
pushd %{py3dir}
%{__python3} setup.py install --skip-build --root %{buildroot}
popd
pushd %{buildroot}%{_bindir}
for f in $(ls); do mv $f python%{python3_pkgversion}-$f; done
popd
%endif # with_python3

%if 0%{?s3_with_python36:1} || 0%{?s3_with_python36_rhel7:1}
pushd %{py3dir}-for36
python3 setup.py install --skip-build --root %{buildroot}
popd
pushd %{buildroot}%{_bindir}
for f in $(ls -1 | grep -v '^python'); do mv $f python36-$f; done
popd
%endif # with_python36

#%{__python} setup.py install --skip-build --root %{buildroot}


%files
%if 0%{?s3_with_python34:1}
%files -n python%{python3_pkgversion}-%{pypi_name}
%doc LICENSE.txt CHANGES.txt README.txt
%{_bindir}/python%{python3_pkgversion}-wheel
%{python3_sitelib}/%{pypi_name}*
%exclude %{python3_sitelib}/%{pypi_name}/test
%endif # with_python3

%if 0%{?s3_with_python36:1} || 0%{?s3_with_python36_rhel7:1}
%files -n python36-%{pypi_name}
%doc LICENSE.txt CHANGES.txt README.txt
%{_bindir}/python36-wheel
%{python3_sitelib}/%{pypi_name}*
%exclude %{python3_sitelib}/%{pypi_name}/test
%endif # with_python36


%changelog
* Sat Jan 03 2015 Matej Cepl <mcepl@redhat.com> - 0.24.0-2
- Make python3 conditional (switched off for RHEL-7; fixes #1131111).

* Mon Nov 10 2014 Slavek Kabrda <bkabrda@redhat.com> - 0.24.0-1
- Update to 0.24.0
- Remove patches merged upstream

* Sun Jun 08 2014 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 0.22.0-4
- Rebuilt for https://fedoraproject.org/wiki/Fedora_21_Mass_Rebuild

* Fri Apr 25 2014 Matej Stuchlik <mstuchli@redhat.com> - 0.22.0-3
- Another rebuild with python 3.4

* Fri Apr 18 2014 Matej Stuchlik <mstuchli@redhat.com> - 0.22.0-2
- Rebuild with python 3.4

* Thu Nov 28 2013 Bohuslav Kabrda <bkabrda@redhat.com> - 0.22.0-1
- Initial package.

