import os
from setuptools import setup
files = ["VERSION"]

# Load the version
s3recovery_version = "1.0.0"
current_script_path = os.path.abspath(os.path.dirname(__file__))
with open(os.path.join(current_script_path, 'VERSION')) as version_file:
    s3recovery_version = version_file.read().strip()

setup(
    # Application name:
    name="s3recovery",

    # Version number (initial):
    version=s3recovery_version,

    # Application author details:
    author="Seagate",

    # Packages
    packages=["s3recovery"],

    # Include additional files into the package
    include_package_data=True,

    # Details
    scripts =['s3recovery/s3recovery'],

    # license="LICENSE.txt",
    description="Recover S3 from metatdata corruption",

    package_data = { 's3recovery': files}

)
