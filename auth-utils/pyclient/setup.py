from distutils.core import setup

requires = [
    'boto3==1.2.2',
    'xmltodict==0.9.2'
]

setup(
    # Application name:
    name="pyclient",

    # Version number (initial):
    version="0.1.0",

    # Application author details:
    author="Arjun Hariharan",
    author_email="arjun.hariharan@seagate.com",

    # Packages
    packages=["pyclient"],

    # Include additional files into the package
    include_package_data=True,
    package_data={
        'config': [
            '*',
        ]
    },

    # Details
    url="",

    #
    # license="LICENSE.txt",
    description="AWS Python client",

    # Dependent packages (distributions)
    install_requires=requires
)
