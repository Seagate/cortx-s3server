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

# _s3_deploy_tag => tag to identify the deployment.
# Example: Cortx01
# _s3_domain_tag => Domain used for S3 service
# Example: s3.h1.motr.colo.seagate.com
# These can also be any useful identifier to know the targeted deployment.
# Example: _s3_deploy_tag = cortx.1.1.singapore or cortx01.lco.1.1

# build number
%define build_num  %( test -n "$build_number" && echo "$build_number" || echo 1 )

Name:           stx-s3-client-certs
Version:        %{_s3_certs_version}
Release:        %{build_num}_%{_s3_deploy_tag}
Summary:        SSL client certificates for S3 [%{_s3_domain_tag}].

License:        Seagate
URL:            https://github.com/Seagate/cortx-s3server
Source:         %{_s3_certs_src}.tar.gz

BuildRequires:  openssl
Requires:       openssl

%description
SSL client certificates for S3 service. Target domain [%{_s3_domain_tag}].

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
%defattr(-,root,root,-)

# s3 client certs
%attr(0444, root, root) /etc/ssl/stx-s3-clients/s3/ca.crt

# s3iamcli client certs
%attr(0444, root, root) /etc/ssl/stx-s3-clients/s3auth/s3authserver.crt
