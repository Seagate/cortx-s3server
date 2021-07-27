# Steps for hosting swagger documentation

1. Install NodeJS

>`yum install nodejs -y `

2. Install http-server for hosting documentation.

>`npm install -g http-server`

3. Clone Swagger-UI

>`git clone https://github.com/swagger-api/swagger-ui`

4. Copy dist directory

	1. This directory contains files used for hosting.

>`cp -r swagger-ui/dist/ /root/S3_AND_IAM_API`

5. Copy S3 and IAM API yaml files under cortx-s3server repo.

>`cp -r cortx-s3server/docs/swagger/s3_api_yaml_files/ /root/S3_AND_IAM_API `

>`cp -r /docs/swagger/s3_api_yaml_files/ /root/S3_AND_IAM_API `

6. Replace index.html in /root/S3_AND_IAM_API with the sample file 'cortx-s3server/docs/swagger/index.html'
  
7. Start http-server

>`http-server -a <ip> -p <port>`
