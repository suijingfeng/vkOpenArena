#!/bin/bash

set -e

# Build the samples.
pushd examples
[ -d build ] || mkdir build
cd build
cmake ..
make -j`nproc`
popd
