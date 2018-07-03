%{!?python_sitelib: %define python_sitelib %(%{__python} -c "from distutils.sysconfig import get_python_lib; print get_python_lib()")}

%global commit 4c2489361d353db1a1815172a6143c8f5a2d1c40
%global shortcommit 4c248936

Name:           s3cmd
Version:        1.6.1
Release:        1%{dist}
Summary:        Tool for accessing Amazon Simple Storage Service

Group:          Applications/Internet
License:        GPLv2
URL:            http://gerrit-sage.mero.colo.seagate.com:8080/s3cmd
# git clone https://github.com/s3tools/s3cmd
# python setup.py sdist
Source0:        %{name}-%{version}-%{shortcommit}.tar.gz
BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)
BuildArch:      noarch
Patch0:         s3cmd_%{version}_max_retries.patch

%if %{!?fedora:16}%{?fedora} < 16 || %{!?rhel:7}%{?rhel} < 7
BuildRequires:  python-devel
%else
BuildRequires:  python2-devel
%endif
%if %{!?fedora:8}%{?fedora} < 8 || %{!?rhel:6}%{?rhel} < 6
# This is in standard library since 2.5
BuildRequires:  python-elementtree
Requires:       python-elementtree
%endif
BuildRequires:  python-dateutil
BuildRequires:  python-setuptools
Requires:       python-dateutil
Requires:       python-magic

%description
S3cmd lets you copy files from/to Amazon S3
(Simple Storage Service) using a simple to use
command line client.


%prep
%setup -q -n %{name}-%{version}-%{shortcommit}
%patch0 -p1

%build


%install
rm -rf $RPM_BUILD_ROOT
S3CMD_PACKAGING=Yes python setup.py install --prefix=%{_prefix} --root=$RPM_BUILD_ROOT
install -d $RPM_BUILD_ROOT%{_mandir}/man1
install -m 644 s3cmd.1 $RPM_BUILD_ROOT%{_mandir}/man1


%clean
rm -rf $RPM_BUILD_ROOT


%files
%defattr(-,root,root,-)
%{_bindir}/s3cmd
%{_mandir}/man1/s3cmd.1*
%{python_sitelib}/S3
%if 0%{?fedora} >= 9 || 0%{?rhel} >= 6
%{python_sitelib}/s3cmd*.egg-info
%endif
%doc NEWS README.md


%changelog
# https://github.com/s3tools/s3cmd/blob/v%{version}/ChangeLog
