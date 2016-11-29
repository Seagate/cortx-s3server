#!/bin/sh -x

rm -rf nodejs
mkdir -p nodejs
cd nodejs
wget https://nodejs.org/dist/v6.9.1/node-v6.9.1-linux-x64.tar.xz
tar -xvf node-v6.9.1-linux-x64.tar.xz
cd ..
