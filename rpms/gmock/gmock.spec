# Copied from https://github.com/google/googlemock
# Copyright 2007, Google Inc.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#     * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
#     * Neither the name of Google Inc. nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


# Google Mock - a framework for writing C++ mock classes.
#
# This file implements Matcher<const string&>, Matcher<string>, and
# utilities for defining matchers.
Summary:        Google C++ Mocking Framework
Name:           gmock
Version:        1.7.0
Release:        1%{?dist}
License:        BSD
Group:          System Environment/Libraries
URL:            https://github.com/google/googlemock
Source0:        %{name}-%{version}.tar.gz
BuildRoot:      %(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)
# gtest is clonned in gmock build so this check is not required here
#BuildRequires:  gtest-devel >= 1.7.0
BuildRequires:  autoconf
BuildRequires:  automake
BuildRequires:  libtool
BuildRequires:  python36
Requires:       gtest >= 1.7.0

%description
Inspired by jMock, EasyMock, and Hamcrest, and designed with C++'s
specifics in mind, Google C++ Mocking Framework (or Google Mock for
short) is a library for writing and using C++ mock classes.

Google Mock:

 o lets you create mock classes trivially using simple macros,
 o supports a rich set of matchers and actions,
 o handles unordered, partially ordered, or completely ordered
   expectations,
 o is extensible by users, and
 o works on Linux, Mac OS X, Windows, Windows Mobile, minGW, and
   Symbian.

# below definition needed, without that on RHEL 8 seeing failure
# error: Empty %files file /root/rpmbuild/BUILD/gmock-1.7.0/debugsourcefiles.list
%global debug_package %{nil}

%package        devel
Summary:        Development files for %{name}
Group:          System Environment/Libraries
Requires:       %{name} = %{version}-%{release}

%description    devel
This package contains development files for %{name}.

%prep
%setup -q

%build
mkdir build
cd build
CFLAGS=-fPIC CXXFLAGS=-fPIC cmake ..

make %{?_smp_mflags}

%install
rm -rf %{buildroot}
install -d $RPM_BUILD_ROOT{%{_bindir},%{_datadir}/aclocal,%{_includedir}/gmock{,/internal},%{_libdir}}
install -p -m 0755 build/libgmock.a build/libgmock_main.a $RPM_BUILD_ROOT%{_libdir}/
/sbin/ldconfig -n $RPM_BUILD_ROOT%{_libdir}
install -p -m 0644 include/gmock/*.h $RPM_BUILD_ROOT%{_includedir}/gmock/
install -p -m 0644 include/gmock/internal/*.h $RPM_BUILD_ROOT%{_includedir}/gmock/internal/

%clean
rm -rf %{buildroot}

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%defattr(-, root, root, -)
%doc CHANGES CONTRIBUTORS LICENSE README
%{_libdir}/libgmock.a
%{_libdir}/libgmock_main.a

%files devel
%defattr(-, root, root, -)
%{_libdir}/libgmock.a
%{_libdir}/libgmock_main.a
%{_includedir}/gmock

%changelog
# Refer: https://github.com/google/googlemock/blob/release-%{version}/CHANGES
