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
rm -rf %{_sourcedir}/%{name}-%{version}
mkdir -p %{_sourcedir}/%{name}-%{version}
cd %{_sourcedir}/%{name}-%{version}
wget https://raw.githubusercontent.com/llvm-mirror/clang/release_60/tools/clang-format/git-clang-format
wget https://raw.githubusercontent.com/llvm-mirror/clang/release_60/LICENSE.TXT
cd %{_sourcedir}
tar -zcvf %{name}-%{version}.tar.gz %{name}-%{version}

%setup -q

%install
cd %{_sourcedir}/%{name}-%{version}
mkdir -p %{buildroot}/usr/bin
cp git-clang-format %{buildroot}/usr/bin
cp LICENSE.TXT %{_builddir}

%clean
rm -rf %{buildroot}

%files -n git-clang-format
%attr(755, root, root) /usr/bin/git-clang-format
%license LICENSE.TXT
%{_bindir}/git-clang-format
