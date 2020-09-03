### License

Copyright (c) 2020 Seagate Technology LLC and/or its Affiliates

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

For any questions about this software or licensing,
please email opensource@seagate.com or cortx-questions@seagate.com.

# Following steps are useful if any config / properties files user do not want overwrite for s3server rpm / yum update

- cd s3server/rpms/s3/
- vim s3rpm.spec
- e.g. if s3.config file is newly added or user want this file will not replace at the time for s3 rpm update then
  following file path need to be add in s3rpm.spec with noreplace tag.

	%config(noreplace) /opt/seagate/cortx/s3/conf/s3.config

- %config(noreplace) will not replace your config file if you changed it.
  If you did not touch the config file, it will always be overwritten with the new config file.

- Following config files currently included for not to overwrite.

	%config(noreplace) /opt/seagate/cortx/auth/resources/authserver.properties
	%config(noreplace) /opt/seagate/cortx/auth/resources/authserver-log4j2.xml
	%config(noreplace) /opt/seagate/cortx/auth/resources/authencryptcli-log4j2.xml
	%config(noreplace) /opt/seagate/cortx/auth/resources/keystore.properties
	%config(noreplace) /opt/seagate/cortx/auth/resources/static/saml-metadata.xml
	%config(noreplace) /opt/seagate/cortx/s3/conf/s3config.yaml
	%config(noreplace) /opt/seagate/cortx/s3/conf/s3server_audit_log.properties
	%config(noreplace) /opt/seagate/cortx/s3/conf/s3_obj_layout_mapping.yaml
	%config(noreplace) /opt/seagate/cortx/s3/conf/s3stats-allowlist.yaml
	%config(noreplace) /opt/seagate/cortx/auth/resources/defaultAclTemplate.xml
	%config(noreplace) /opt/seagate/cortx/auth/resources/AmazonS3.xsd
	%config(noreplace) /opt/seagate/cortx/s3/s3backgrounddelete/config.yaml


# Before rpm upadtes steps:
	1. Before rpm / yum updates user need to shutdown s3 services, authserver, slapd, backgrounddelete services.

# Config files backup (optional):

        1. User can take the following folder backup before update of the s3 server rpm.
                 /opt/seagate/cortx/auth/resources/
                 /opt/seagate/cortx/s3/conf/
                 /opt/seagate/cortx/s3/s3backgrounddelete/

# How to Upgrade a s3 server RPM Package:

	1. if user have updated version of the s3 rpm then following command is useful for upgrade:
		rpm -Uvh s3server-xxx.rpm
	2. Alternatively user can also update on the latest rpm using yum command.
		yum update s3server

# After rpm upadtes complete:
        1. After rpm / yum updates user need to start s3 services, authserver, slapd, backgrounddelete services.

