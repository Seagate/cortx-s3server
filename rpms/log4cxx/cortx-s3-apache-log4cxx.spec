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

Name:		log4cxx_cortx
Version:	0.10.0
Release:	1
Summary:	A logging framework for C++ patterned after Apache log4j

Group:		Development/Libraries/C and C++
License:	Apache
URL:		http://logging.apache.org/log4cxx
Source0:	apache-log4cxx-%{version}.tar.bz2
Patch0: 	apache-log4cxx-%{version}.patch
Prefix: 	%_prefix
BuildRoot: 	%{_tmppath}/apache-log4cxx-%{version}-build
Requires:	unixODBC
Requires:	apr
Requires:	apr-util

%description
Apache log4cxx is a logging framework for C++ patterned after Apache log4j.
As Apache log4cxx uses Apache portable runtime for most platform specific code,
it should be usable on any APR supported plaform.

%package -n log4cxx_cortx-devel
License:        Apache
Group:          Development/Libraries/C and C++
Requires:       %{name} = %{version}
Summary:        The development files for log4cxx

%description -n log4cxx_cortx-devel
Apache log4cxx is a logging framework for C++, based on Apache log4j.
As Apache log4cxx uses Apache portable runtime for most platofrm specific code,
it should be usable on any APR supported plaform.

%prep
%setup -n apache-log4cxx-%{version}
%patch0 -p1

%build
./autogen.sh
./configure --prefix=/usr --libdir=%{_libdir}
make

%install
make install prefix=$RPM_BUILD_ROOT%{prefix} libdir=$RPM_BUILD_ROOT%{_libdir}

# files
cd $RPM_BUILD_ROOT
find .%{_includedir}/log4cxx -print | sed 's,^\.,\%attr(-\,root\,root) ,' >  $RPM_BUILD_DIR/file.list
find .%{_datadir}/log4cxx    -print | sed 's,^\.,\%attr(-\,root\,root) ,' >> $RPM_BUILD_DIR/file.list


%clean
rm -rf $RPM_BUILD_ROOT
rm -rf $RPM_BUILD_DIR/%{name}-%{version}
rm $RPM_BUILD_DIR/file.list

%post   -p /sbin/ldconfig

%postun -p /sbin/ldconfig


%files
%defattr(-,root,root,-)
%{_libdir}/liblog4cxx.so.10
%{_libdir}/liblog4cxx.so.10.0.0

%files -n log4cxx_cortx-devel -f ../file.list
%defattr(-,root,root,-)
%{_libdir}/liblog4cxx.a
%{_libdir}/liblog4cxx.la
%{_libdir}/liblog4cxx.so
%{_libdir}/pkgconfig/liblog4cxx.pc

