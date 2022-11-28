#!/bin/bash
# run this from repo root directory
# bash scripts/install-library.sh
sudo apt-get update
sudo apt-get install -y libprotobuf-dev libboost-all-dev liblog4cpp5-dev
cd third-party/libconfig
mkdir build
cd build
cmake ..
make
sudo make install
cd ../../..