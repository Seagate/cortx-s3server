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

%global _enable_debug_package 0
%global debug_package %{nil}
%global __os_install_post /usr/lib/rpm/brp-compress %{nil}

Name:		bazel
Version:	0.13.0
Release:	1%{?dist}
Summary:	Build tool

Group:		Development/Tools
License:	Apache
URL:		https://github.com/bazelbuild/bazel
Source0:	%{name}-%{version}.zip

BuildRequires:	java-1.8.0-openjdk-devel
Requires:	java-1.8.0-openjdk-devel

%description
Google build tool

%prep
%setup -c -n %{name}-%{version}

%build
./compile.sh

%install
mkdir -p %{buildroot}/usr/bin
cp ./output/bazel %{buildroot}/usr/bin
cp LICENSE %{_builddir}

%clean
rm -rf ${buildroot}

%files
%defattr(-,root,root)
%license LICENSE
/usr/bin/bazel

