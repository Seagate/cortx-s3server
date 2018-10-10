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

%files

# S3 server certificates
/etc/ssl/stx-s3/s3/ca.crt
/etc/ssl/stx-s3/s3/ca.key
/etc/ssl/stx-s3/s3/s3server.crt
/etc/ssl/stx-s3/s3/s3server.csr
/etc/ssl/stx-s3/s3/s3server.key
/etc/ssl/stx-s3/s3/s3server.pem

# S3 Auth server certificates
/etc/ssl/stx-s3/s3auth/s3authserver.crt
/etc/ssl/stx-s3/s3auth/s3authserver.pem
/etc/ssl/stx-s3/s3auth/s3authserver.jks.key
/etc/ssl/stx-s3/s3auth/s3authserver.p12
/etc/ssl/stx-s3/s3auth/s3authserver.key
/etc/ssl/stx-s3/s3auth/s3authserver.jks
/etc/ssl/stx-s3/s3auth/s3authserver.jks.pem

# S3 openldap server certificates
/etc/ssl/stx-s3/openldap/ca.crt
/etc/ssl/stx-s3/openldap/ca.key
/etc/ssl/stx-s3/openldap/s3openldap.crt
/etc/ssl/stx-s3/openldap/s3openldap.csr
/etc/ssl/stx-s3/openldap/s3openldap.key
