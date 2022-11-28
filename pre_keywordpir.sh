#/bin/bash
# Install keyword PIR dependencies

cpu_num=`lscpu | grep "CPU:" | awk '{print $2}'`
cd /tmp
wget https://github.com/zeromq/libzmq/archive/refs/tags/v4.3.4.tar.gz
tar -zxf v4.3.4.tar.gz
cd libzmq-4.3.4/ && mkdir build && cd build && cmake .. && make -j $cpu_num && make install

wget https://github.com/zeromq/cppzmq/archive/refs/tags/v4.9.0.tar.gz
tar -zxf v4.9.0.tar.gz
cd cppzmq-4.9.0/ && mkdir build && cd build/ && cmake .. && make -j $cpu_num install

wget https://github.com/google/flatbuffers/archive/refs/tags/v2.0.0.tar.gz
tar -zxf v2.0.0.tar.gz
cd flatbuffers-2.0.0/ && mkdir build && cd build/ && cmake .. && make -j $cpu_num && make instal

wget https://sourceforge.net/projects/tclap/files/tclap-1.2.5.tar.gz
tar -zxvf tclap-1.2.5.tar.gz
cd tclap-1.2.5 && ./configure && make -j $cpu_num && make install

echo "Install keyword PIR dependencies success!"