# _s3_domain_tag => tag to identify the deployment domain.
# Example: h1.mero.colo.seagate.com
# It can also be any useful identifier to know the targeted deployment.
# Example: hermi.1.1.singapore or hermi01.lco.1.1

# build number
%define build_num  %( test -n "$build_number" && echo "$build_number" || echo 1 )

Name:           stx-s3-client-certs
Version:        %{_s3_certs_version}
Release:        %{build_num}_%{?dist:el7}
Summary:        Certificates for running clients against Seagate S3 for %{_s3_domain_tag}.

License:        Seagate
URL:            http://gerrit.mero.colo.seagate.com:8080/s3server
Source:         %{_s3_certs_src}.tar.gz

BuildRequires:  openssl
Requires:       openssl

%description
Certificates for running clients against Seagate S3 for %{_s3_domain_tag}.

%prep
%setup -n %{_s3_certs_src}

%install
rm -rf %{buildroot}
install -d $RPM_BUILD_ROOT/etc/ssl/stx-s3-clients/{s3,s3auth}

install -p s3/ca.crt $RPM_BUILD_ROOT/etc/ssl/stx-s3-clients/s3/ca.crt

install -p s3auth/s3authserver.crt $RPM_BUILD_ROOT/etc/ssl/stx-s3-clients/s3auth/s3authserver.crt

%clean
rm -rf %{buildroot}

%files

# s3 client certs
/etc/ssl/stx-s3-clients/s3/ca.crt

# s3iamcli client certs
/etc/ssl/stx-s3-clients/s3auth/s3authserver.crt
