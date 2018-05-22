from setuptools import setup
files = ["config/*"]
setup(
    # Application name:
    name="s3iamcli",

    # Version number (initial):
    version="1.0.0",

    # Application author details:
    author="Seagate",

    # Packages
    packages=["s3iamcli"],

    # Include additional files into the package
    include_package_data=True,

    # Details
    scripts =['s3iamcli/s3iamcli'],

    # license="LICENSE.txt",
    description="Seaget S3 IAM CLI.",

    package_data = { 's3iamcli': files},

    # Dependent packages (distributions)
    install_requires=[
    'boto3',
    'botocore',
    'xmltodict'
  ]
)
