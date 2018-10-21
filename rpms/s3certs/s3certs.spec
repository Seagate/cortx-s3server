# _s3_domain_tag => tag to identify the deployment domain.
# Example: h1.mero.colo.seagate.com
# It can also be any useful identifier to know the targeted deployment.
# Example: hermi.1.1.singapore or hermi01.lco.1.1

# build number
%define build_num  %( test -n "$build_number" && echo "$build_number" || echo 1 )

Name:           stx-s3-certs
Version:        %{_s3_certs_version}
Release:        %{build_num}_%{?dist:el7}
Summary:        Certificates for running Seagate S3 services for %{_s3_domain_tag}.

License:        Seagate
URL:            http://gerrit.mero.colo.seagate.com:8080/s3server
Source:         %{_s3_certs_src}.tar.gz

BuildRequires:  openssl
Requires:       openssl

%description
Certificates for running Seagate S3 services with %{_s3_domain_tag} domain.

%prep
%setup -n %{name}-%{version}-%{_s3_domain_tag}

%install
rm -rf %{buildroot}

install -d $RPM_BUILD_ROOT/etc/ssl/stx-s3/{s3,s3auth,openldap}
install -p s3/* $RPM_BUILD_ROOT/etc/ssl/stx-s3/s3
install -p s3auth/* $RPM_BUILD_ROOT/etc/ssl/stx-s3/s3auth
install -p openldap/* $RPM_BUILD_ROOT/etc/ssl/stx-s3/openldap

%clean
rm -rf %{buildroot}

%pre
# Ensure we have users "ldap", "haproxy" created by mentioned packages.
getent passwd haproxy 2>/dev/null || { \
  echo -e "\n*ERROR* User 'haproxy' missing. Please install haproxy package.";
  exit 1; }

getent passwd ldap 2>/dev/null || { \
  echo -e "\n*ERROR* User 'ldap' missing. Please install openldap-servers package.";
  exit 1; }

%files
%defattr(-,root,root,-)

# S3 server certificates
%attr(0751, haproxy, haproxy) /etc/ssl/stx-s3/s3/ca.crt
%attr(0751, haproxy, haproxy) /etc/ssl/stx-s3/s3/ca.key
%attr(0751, haproxy, haproxy) /etc/ssl/stx-s3/s3/s3server.crt
%attr(0751, haproxy, haproxy) /etc/ssl/stx-s3/s3/s3server.csr
%attr(0751, haproxy, haproxy) /etc/ssl/stx-s3/s3/s3server.key
%attr(0751, haproxy, haproxy) /etc/ssl/stx-s3/s3/s3server.pem

# S3 Auth server certificates
%attr(0751, root, root) /etc/ssl/stx-s3/s3auth/s3authserver.crt
%attr(0751, root, root) /etc/ssl/stx-s3/s3auth/s3authserver.pem
%attr(0751, root, root) /etc/ssl/stx-s3/s3auth/s3authserver.jks.key
%attr(0751, root, root) /etc/ssl/stx-s3/s3auth/s3authserver.p12
%attr(0751, root, root) /etc/ssl/stx-s3/s3auth/s3authserver.key
%attr(0751, root, root) /etc/ssl/stx-s3/s3auth/s3authserver.jks
%attr(0751, root, root) /etc/ssl/stx-s3/s3auth/s3authserver.jks.pem

# S3 openldap server certificates
%attr(0751, ldap, ldap) /etc/ssl/stx-s3/openldap/ca.crt
%attr(0751, ldap, ldap) /etc/ssl/stx-s3/openldap/ca.key
%attr(0751, ldap, ldap) /etc/ssl/stx-s3/openldap/s3openldap.crt
%attr(0751, ldap, ldap) /etc/ssl/stx-s3/openldap/s3openldap.csr
%attr(0751, ldap, ldap) /etc/ssl/stx-s3/openldap/s3openldap.key
