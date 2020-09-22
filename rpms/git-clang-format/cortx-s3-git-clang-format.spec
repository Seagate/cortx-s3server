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

Name:		git-clang-format
Version:	6.0
Release:	1%{?dist}
Summary:	clang-format integration for git

License:	NCSA
URL:		http://llvm.org
Source0:	%{name}-%{version}.tar.gz
Requires:	clang >= 3.4
Requires:	wget
Requires:	git

%description -n git-clang-format
clang-format integration for git.

%prep
%setup -q

%install
rm -rf %{buildroot}
install -d $RPM_BUILD_ROOT%{_bindir}/
cp git-clang-format $RPM_BUILD_ROOT%{_bindir}/
cp LICENSE.TXT %{_builddir}

%clean
rm -rf %{buildroot}

%files -n git-clang-format
%attr(755, root, root) /usr/bin/git-clang-format
%license LICENSE.TXT
%{_bindir}/git-clang-format

