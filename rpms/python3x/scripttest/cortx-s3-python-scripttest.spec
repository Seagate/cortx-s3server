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

# Disable documentation generation for now
%bcond_with docs

Name:		python-scripttest
Version:	1.3.0
Release:	18%{?dist}
Summary:	Helper library to test cmd-line scripts

License:	MIT
URL:		http://pypi.python.org/pypi/ScriptTest
Source0:	https://github.com/pypa/scripttest/archive/1.3.0.tar.gz
BuildArch:      noarch
BuildRequires:	python%{python3_pkgversion}-devel
BuildRequires: python%{python3_pkgversion}-setuptools
%if %{with docs}
BuildRequires: python%{python3_pkgversion}-sphinx
%endif
%if %{with test}
BuildRequires: python%{python3_pkgversion}-pytest
%endif

%description
library to test interactive cmd-line applications.

%package -n     python36-scripttest
Summary:        Helper to test command-line scripts
%{?python_provide:%python_provide python36-scripttest}

%description -n python36-scripttest
library to test interactive cmd-line applications

%prep
%setup -q -n scripttest-%{version}


%build
%py3_build
%if %{with docs}
sphinx-build -b html docs/ docs/html
%endif


%install
%py3_install
%check
%{__python3} setup.py test

%files -n python36-scripttest
%if %{with docs}
%doc docs/html
%endif
%license docs/license.rst
%{python3_sitelib}/scripttest.py
%{python3_sitelib}/__pycache__/scripttest.cpython-??*
%{python3_sitelib}/scripttest*.egg-info/

