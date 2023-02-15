#！/bin/bash

bash pre_build.sh
bazel build --config=linux_x86_64 :libpsi_test

cd ./bazel-bin

echo -e "\e[44m Limit bandwidth to 100mbps \e[0m"

## replace lo with your card 
sudo tc qdisc del dev lo root
## about 100mbps
sudo tc qdisc add dev lo root handle 1: tbf rate 100mbit burst 100000 limit 10000
## about 0.3ms ping latency
sudo tc qdisc add dev lo parent 1:1 handle 10: netem delay 0.15msec



echo -e "\e[31m kkrt, mkkrt, cm20 benchmark under single thread \e[0m"

echo -e "\e[32m Dataset size: 10 million: 100,000 \e[0m"
./libpsi_test -kkrt -ss 10000000 -rs 100000 
./libpsi_test -mkkrt -ss 10000000 -rs 100000 
./libpsi_test -cm20 -ss 10000000 -rs 100000 

echo -e "\e[32m Dataset size: 100 million: 1 million \e[0m"
./libpsi_test -kkrt -ss 100000000 -rs 1000000 
./libpsi_test -mkkrt -ss 100000000 -rs 1000000 
./libpsi_test -cm20 -ss 100000000 -rs 1000000 


echo -e "\e[32m Dataset size: 100 million: 1 million \e[0m"
./libpsi_test -kkrt -ss 10000000 -rs 10000000 
./libpsi_test -mkkrt -ss 10000000 -rs 10000000 
./libpsi_test -cm20 -ss 10000000 -rs 10000000 


echo -e "\e[32m Dataset size: 100 million: 1 million \e[0m"
./libpsi_test -kkrt -ss 100000000 -rs 100000000 
./libpsi_test -mkkrt -ss 100000000 -rs 100000000 
./libpsi_test -cm20 -ss 100000000 -rs 100000000 



echo -e "\e[31m kkrt, mkkrt, cm20 benchmarks with 16 threads \e[0m"


echo -e "\e[32m Dataset size: 100 million: 1 million \e[0m"
./libpsi_test -kkrt -ss 10000000 -rs 100000 -t 16
./libpsi_test -mkkrt -ss 10000000 -rs 100000 -t 16
./libpsi_test -cm20 -ss 10000000 -rs 100000  -t 16


echo -e "\e[32m Dataset size: 100 million: 1 million \e[0m"
./libpsi_test -kkrt -ss 100000000 -rs 1000000 -t 16
./libpsi_test -mkkrt -ss 100000000 -rs 1000000 -t 16
./libpsi_test -cm20 -ss 100000000 -rs 1000000 -t 16


echo -e "\e[32m Dataset size: 100 million: 1 million \e[0m"
./libpsi_test -kkrt -ss 10000000 -rs 10000000 -t 16
./libpsi_test -mkkrt -ss 10000000 -rs 10000000 -t 16
./libpsi_test -cm20 -ss 10000000 -rs 10000000 -t 16


echo -e "\e[32m Dataset size: 100 million: 1 million \e[0m"
./libpsi_test -kkrt -ss 100000000 -rs 100000000 -t 16
./libpsi_test -mkkrt -ss 100000000 -rs 100000000 -t 16
./libpsi_test -cm20 -ss 100000000 -rs 100000000 -t 16



echo -e "\e[44m Limit bandwidth to 1000mbps \e[0m"
## replace lo with your card 
sudo tc qdisc del dev lo root
## about 1000mbps
sudo tc qdisc add dev lo root handle 1: tbf rate 1000mbit burst 100000 limit 10000
## about 0.3ms ping latency
sudo tc qdisc add dev lo parent 1:1 handle 10: netem delay 0.15msec


echo -e "\e[31m kkrt, mkkrt, cm20 benchmarks under single thread \e[0m"

echo -e "\e[32m Dataset size: 10 million: 100,000 \e[0m"
./libpsi_test -kkrt -ss 10000000 -rs 100000 
./libpsi_test -mkkrt -ss 10000000 -rs 100000 
./libpsi_test -cm20 -ss 10000000 -rs 100000 

echo -e "\e[32m Dataset size: 100 million: 1 million \e[0m"
./libpsi_test -kkrt -ss 100000000 -rs 1000000 
./libpsi_test -mkkrt -ss 100000000 -rs 1000000 
./libpsi_test -cm20 -ss 100000000 -rs 1000000 

echo -e "\e[32m Dataset size: 10 million: 10 million \e[0m"
./libpsi_test -kkrt -ss 10000000 -rs 10000000 
./libpsi_test -mkkrt -ss 10000000 -rs 10000000 
./libpsi_test -cm20 -ss 10000000 -rs 10000000 


echo -e "\e[32m Dataset size: 100 million: 100million \e[0m"
./libpsi_test -kkrt -ss 100000000 -rs 100000000 
./libpsi_test -mkkrt -ss 100000000 -rs 100000000 
./libpsi_test -cm20 -ss 100000000 -rs 100000000 



echo -e "\e[31m kkrt, mkkrt, cm20 benchmarks with 16 threads \e[0m"


echo -e "\e[32m Dataset size: 10 million: 100,000 \e[0m"
./libpsi_test -kkrt -ss 10000000 -rs 100000 -t 16
./libpsi_test -mkkrt -ss 10000000 -rs 100000 -t 16
./libpsi_test -cm20 -ss 10000000 -rs 100000  -t 16


echo -e "\e[32m Dataset size: 100 million: 1 million \e[0m"
./libpsi_test -kkrt -ss 100000000 -rs 1000000 -t 16
./libpsi_test -mkkrt -ss 100000000 -rs 1000000 -t 16
./libpsi_test -cm20 -ss 100000000 -rs 1000000 -t 16


echo -e "\e[32m Dataset size: 10 million: 10 million \e[0m"
./libpsi_test -kkrt -ss 10000000 -rs 10000000 -t 16
./libpsi_test -mkkrt -ss 10000000 -rs 10000000 -t 16
./libpsi_test -cm20 -ss 10000000 -rs 10000000 -t 16


echo -e "\e[32m Dataset size: 100 million: 100 million \e[0m"
./libpsi_test -kkrt -ss 100000000 -rs 100000000 -t 16
./libpsi_test -mkkrt -ss 100000000 -rs 100000000 -t 16
./libpsi_test -cm20 -ss 100000000 -rs 100000000 -t 16


echo -e "\e[31m kkrt, mkkrt, cm20 benchmarks with 36 threads \e[0m"


echo -e "\e[32m Dataset size: 10 million: 100,000 \e[0m"
./libpsi_test -kkrt -ss 10000000 -rs 100000 -t 36
./libpsi_test -mkkrt -ss 10000000 -rs 100000 -t 36
./libpsi_test -cm20 -ss 10000000 -rs 100000  -t 36


echo -e "\e[32m Dataset size: 10 million: 1 million \e[0m"
./libpsi_test -kkrt -ss 100000000 -rs 1000000 -t 36
./libpsi_test -mkkrt -ss 100000000 -rs 1000000 -t 36
./libpsi_test -cm20 -ss 100000000 -rs 1000000 -t 36


echo -e "\e[32m Dataset size: 10 million: 10 million \e[0m"
./libpsi_test -kkrt -ss 10000000 -rs 10000000 -t 36
./libpsi_test -mkkrt -ss 10000000 -rs 10000000 -t 36
./libpsi_test -cm20 -ss 10000000 -rs 10000000 -t 36


echo -e "\e[32m Dataset size: 100 million: 100 million \e[0m"
./libpsi_test -kkrt -ss 100000000 -rs 100000000 -t 36
./libpsi_test -mkkrt -ss 100000000 -rs 100000000 -t 36
./libpsi_test -cm20 -ss 100000000 -rs 100000000 -t 36
