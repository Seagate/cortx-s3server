#!/bin/sh -x

rm -rf statsd

git clone -b v0.8.0 --depth=1 https://github.com/etsy/statsd.git
