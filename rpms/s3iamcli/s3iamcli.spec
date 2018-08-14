
# build number
%define build_num  %( test -n "$build_number" && echo "$build_number" || echo 1 )

Name:       s3iamcli
Version:    %{_s3iamcli_version}
Release:    %{build_num}_%{_s3iamcli_git_ver}
Summary:    Seagate S3 IAM CLI.

Group:      Development/Tools
License:    Seagate
URL:        http://gerrit.mero.colo.seagate.com:8080/s3server
Source0:    %{name}-%{version}-%{_s3iamcli_git_ver}.tar.gz

BuildRoot:  %{_tmppath}/%{name}-%{version}-%{release}-buildroot
Prefix:     %{_prefix}
BuildArch:  noarch
Vendor:     Seagate

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

%package        devel
Summary:        Development files for %{name}
Group:          Development/Tools

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

%description    devel
This package contains development files for %{name}.


%prep
%setup -n %{name}-%{version}-%{_s3iamcli_git_ver}

%build
mkdir -p %{_builddir}/%{name}-%{version}-%{_s3iamcli_git_ver}/build/lib/%{name}
cd %{name}
python3 -m compileall -b *.py
cp  *.pyc %{_builddir}/%{name}-%{version}-%{_s3iamcli_git_ver}/build/lib/%{name}
echo "build complete"

%install
cd %{_builddir}/%{name}-%{version}-%{_s3iamcli_git_ver}
python3 setup.py install --no-compile --single-version-externally-managed -O1 --root=$RPM_BUILD_ROOT

%clean
rm -rf $RPM_BUILD_ROOT

%files
%{_bindir}/%{name}
%{python3_sitelib}/%{name}/config/*.yaml
%{python3_sitelib}/%{name}-%{version}-py?.?.egg-info
%{python3_sitelib}/%{name}/*.pyc
%exclude %{python3_sitelib}/%{name}/__pycache__/*
%exclude %{python3_sitelib}/%{name}/*.py
%exclude %{python3_sitelib}/%{name}/%{name}
%defattr(-,root,root)

%files devel
%{_bindir}/%{name}
%{python3_sitelib}/%{name}/*.py
%{python3_sitelib}/%{name}/config/*.yaml
%{python3_sitelib}/%{name}-%{version}-py?.?.egg-info
%exclude %{python3_sitelib}/%{name}/*.pyc
%exclude %{python3_sitelib}/%{name}/__pycache__/*
%exclude %{python3_sitelib}/%{name}/%{name}
%defattr(-,root,root)
