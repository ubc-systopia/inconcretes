git submodule update --init --recursive

sudo apt-get update
sudo apt-get install cmake libhiredis-dev redis-server libprotobuf-dev

mkdir temp

pushd temp
git clone https://github.com/sewenew/redis-plus-plus.git
cd redis-plus-plus
mkdir build
cd build
cmake -DREDIS_PLUS_PLUS_CXX_STANDARD=17 ..
make
sudo make install
popd

pushd temp
wget https://versaweb.dl.sourceforge.net/project/log4cpp/log4cpp-1.1.x%20%28new%29/log4cpp-1.1/log4cpp-1.1.3.tar.gz
tar -xf log4cpp-1.1.3.tar.gz
cd log4cpp
./configure
make
make check
sudo make install
popd

pushd temp
wget https://github.com/hyperrealm/libconfig/archive/refs/tags/v1.7.3.tar.gz
tar -xf v1.7.3.tar.gz
cd libconfig-1.7.3
mkdir build
cd build
cmake ..
make
sudo make install
popd
