%define name s3iamcli
%define version 1.0.0
%define release 1

Summary: Seagate S3 IAM CLI.
Name: %{name}
Version: %{version}
Release: %{release}
Source0: %{name}-%{version}.tar.gz
URL: http://gerrit-sage.dco.colo.seagate.com:8080/s3server
License: Seagate
Group: Development/Tools
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-buildroot
Prefix: %{_prefix}
BuildArch: noarch
Vendor: Seagate

BuildRequires:  python%{python3_pkgversion} >= 3.4
BuildRequires:  python%{python3_pkgversion}-devel
BuildRequires:  python%{python3_pkgversion}-setuptools

Requires:  python%{python3_pkgversion} >= 3.4
Requires:  python%{python3_pkgversion}-yaml
Requires:  python%{python3_pkgversion}-xmltodict >= 0.9.0
Requires:  python%{python3_pkgversion}-jmespath >= 0.7.1
Requires:  python%{python3_pkgversion}-botocore >= 1.5.0
Requires:  python%{python3_pkgversion}-s3transfer >= 0.1.10
Requires:  python%{python3_pkgversion}-boto3 >= 1.4.6

%description
Seagate S3 IAM CLI

%prep
%setup -n %{name}-%{version}

%build
python3 setup.py build

%install
python3 setup.py install --single-version-externally-managed -O1 --root=$RPM_BUILD_ROOT --record=INSTALLED_FILES

%clean
rm -rf $RPM_BUILD_ROOT

%files -f INSTALLED_FILES
%defattr(-,root,root)
