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

%if 0%{?s3_with_python34:1}%{?s3_with_python36_ver8:1}%{?s3_with_python36_rhel7:1} != 0
%{!?py3ver: %global py3ver %(%{__python3} -c "import sys ; print(sys.version[:3])")}
%else
%{!?__python2: %global __python2 /usr/bin/python2}
%{!?python2_sitelib: %global python2_sitelib %(%{__python2} -c "from distutils.sysconfig import get_python_lib; print (get_python_lib())")}
%endif
%{!?py2ver: %global py2ver %(%{__python2} -c "import sys ; print sys.version[:3]")}

%if 0%{?__python3_other:1}
%{!?py3ver_other: %global py3ver_other %(%{__python3_other} -c "import sys ; print(sys.version[:3])")}
%endif # have python3_other

%global srcname xmltodict

%if 0%{?s3_with_python36_ver8:1} || 0%{?s3_with_python36_rhel7:1}
Name:		python36-xmltodict
%else
Name:               python-xmltodict
%endif
Version:	0.9.0
Release:	1%{?dist}
Summary:	Makes working with XML feel like you are working with JSON

Group:		Development/Libraries
License:	MIT
URL:		https://github.com/martinblech/xmltodict
Source0:	http://pypi.python.org/packages/source/x/%{srcname}/%{srcname}-%{version}.tar.gz
BuildArch:          noarch
%if 0%{?s3_with_python36_ver8:1}
BuildRequires:      python3-devel
BuildRequires:      python3-nose
%else
BuildRequires:      python2-devel
BuildRequires:      python-nose
%endif

%if 0%{?s3_with_python34:1}
BuildRequires:      python%{python3_pkgversion}-devel
BuildRequires:      python%{python3_pkgversion}-nose
%endif

%if 0%{?s3_with_python36:1} || 0%{?s3_with_python36_rhel7:1}
BuildRequires:  python3-devel
BuildRequires:  python36-nose
%endif # with_python36

%description
xmltodict is a Python module that makes working with XML feel like you are
working with JSON. It's very fast (Expat-based) and has a streaming mode
with a small memory footprint, suitable for big XML dumps like Discogs or
Wikipedia.

    >>> doc = xmltodict.parse("""
    ... <mydocument has="an attribute">
    ...   <and>
    ...     <many>elements</many>
    ...     <many>more elements</many>
    ...   </and>
    ...   <plus a="complex">
    ...     element as well
    ...   </plus>
    ... </mydocument>
    ... """)
    >>>
    >>> doc['mydocument']['@has']
    u'an attribute'
    >>> doc['mydocument']['and']['many']
    [u'elements', u'more elements']
    >>> doc['mydocument']['plus']['@a']
    u'complex'
    >>> doc['mydocument']['plus']['#text']
    u'element as well'


%if 0%{?s3_with_python34:1}
%package -n python%{python3_pkgversion}-xmltodict
Summary:            Makes working with XML feel like you are working with JSON
Group:              Development/Libraries

Requires:           python%{python3_pkgversion}

%description -n python%{python3_pkgversion}-xmltodict
xmltodict is a Python module that makes working with XML feel like you are
working with JSON. It's very fast (Expat-based) and has a streaming mode
with a small memory footprint, suitable for big XML dumps like Discogs or
Wikipedia.

    >>> doc = xmltodict.parse("""
    ... <mydocument has="an attribute">
    ...   <and>
    ...     <many>elements</many>
    ...     <many>more elements</many>
    ...   </and>
    ...   <plus a="complex">
    ...     element as well
    ...   </plus>
    ... </mydocument>
    ... """)
    >>>
    >>> doc['mydocument']['@has']
    u'an attribute'
    >>> doc['mydocument']['and']['many']
    [u'elements', u'more elements']
    >>> doc['mydocument']['plus']['@a']
    u'complex'
    >>> doc['mydocument']['plus']['#text']
    u'element as well'
%endif

%if 0%{?s3_with_python36:1}
%package -n python36-xmltodict
Summary:            Makes working with XML feel like you are working with JSON
Group:              Development/Libraries

Requires:           python36

%description -n python36-xmltodict
xmltodict is a Python module that makes working with XML feel like you are
working with JSON. It's very fast (Expat-based) and has a streaming mode
with a small memory footprint, suitable for big XML dumps like Discogs or
Wikipedia.

    >>> doc = xmltodict.parse("""
    ... <mydocument has="an attribute">
    ...   <and>
    ...     <many>elements</many>
    ...     <many>more elements</many>
    ...   </and>
    ...   <plus a="complex">
    ...     element as well
    ...   </plus>
    ... </mydocument>
    ... """)
    >>>
    >>> doc['mydocument']['@has']
    u'an attribute'
    >>> doc['mydocument']['and']['many']
    [u'elements', u'more elements']
    >>> doc['mydocument']['plus']['@a']
    u'complex'
    >>> doc['mydocument']['plus']['#text']
    u'element as well'
%endif # with_python36

%if 0%{?s3_with_python36_ver8:1} || 0%{?s3_with_python36_rhel7:1}
#%package -n python3-xmltodict

Requires:           python36

%endif # with_python36_ver8 || with_python36_rhel7

%prep
%setup -q -n %{srcname}-%{version}
rm -rf %{srcname}.egg-info

%if 0%{?s3_with_python34:1}
rm -rf %{py3dir}
cp -a . %{py3dir}
find %{py3dir} -name '*.py' | xargs sed -i '1s|^#!python|#!%{__python3}|'
%endif

%if 0%{?s3_with_python36:1}
rm -rf %{py3dir}-for36
cp -a . %{py3dir}-for36
find %{py3dir}-for36 -name '*.py' | xargs sed -i '1s|^#!python|#!%{__python3_other}|'
%endif # with_python36

%if 0%{?s3_with_python36_ver8:1} || 0%{?s3_with_python36_rhel7:1}
rm -rf %{py3dir}
cp -a . %{py3dir}
find %{py3dir} -name '*.py' | xargs sed -i '1s|^#!python|#!%{__python3}|'
%endif


%build
%if 0%{?s3_with_python36_ver8:1} || 0%{?s3_with_python36_rhel7:1}
%{__python3} setup.py build
%else
%{__python2} setup.py build
%endif
%if 0%{?s3_with_python34:1}
pushd %{py3dir}
%{__python3} setup.py build
popd
%endif
%if 0%{?s3_with_python36:1}
pushd %{py3dir}-for36
%{__python3} setup.py build
popd
%endif # with_python36
%if 0%{?s3_with_python36_ver8:1} || 0%{?s3_with_python36_rhel7:1}
pushd %{py3dir}
%{__python3} setup.py build
popd
%endif #with_python36_ver8 || with_python36_rhel7

%install
%if 0%{?s3_with_python36:1}
pushd %{py3dir}-for36
python3 setup.py install -O1 --skip-build --root=%{buildroot}
popd
%endif # with_python36
%if 0%{?s3_with_python34:1}
pushd %{py3dir}
%{__python3} setup.py install -O1 --skip-build --root=%{buildroot}
popd
%endif
%if 0%{?s3_with_python36_ver8:1} || 0%{?s3_with_python36_rhel7:1}
pushd %{py3dir}
python3 setup.py install -O1 --skip-build --root=%{buildroot}
popd
%endif

%check
%if 0%{?s3_with_python36_ver8:1}
nosetests-3
%else
nosetests-%{py2ver}
%endif
%if 0%{?s3_with_python34:1}
pushd %{py3dir}
nosetests-%{py3ver}
popd
%endif
%if 0%{?s3_with_python36:1}
pushd %{py3dir}-for36
nosetests-3.6
popd
%endif # with_python36
%if 0%{?s3_with_python36_ver8:1}
pushd %{py3dir}
nosetests-3
popd
%endif # with_python36_ver8
%if 0%{?s3_with_python36_rhel7:1}
pushd %{py3dir}
nosetests-%{py3ver}
popd
%endif # with_python36_rhel7


%if 0%{?s3_with_python34:1}
%files -n python%{python3_pkgversion}-xmltodict
%doc README.md LICENSE PKG-INFO
%{python3_sitelib}/%{srcname}.py
%{python3_sitelib}/%{srcname}-%{version}-*
%{python3_sitelib}/__pycache__/%{srcname}*
%endif

%if 0%{?s3_with_python36:1}
%files -n python36-xmltodict
%doc README.md LICENSE PKG-INFO
%{python3_sitelib}/%{srcname}.py
%{python3_sitelib}/%{srcname}-%{version}-*
%{python3_sitelib}/__pycache__/%{srcname}*
%endif # with_python36

%if 0%{?s3_with_python36_ver8:1} || 0%{?s3_with_python36_rhel7:1}
%files -n python36-xmltodict
%doc README.md LICENSE PKG-INFO
%{python3_sitelib}/%{srcname}.py
%{python3_sitelib}/%{srcname}-%{version}-*
%{python3_sitelib}/__pycache__/%{srcname}*
%endif

