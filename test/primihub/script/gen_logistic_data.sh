set -x
set -e

cp ./csv_utils.py /tmp/
cd /tmp

set +e
rm -rf cls_train_x.csv
rm -rf cls_train_y.csv
rm -rf cls_test_x.csv
rm -rf cls_test_y.csv
set -e

# Download train dataset.
wget https://raw.githubusercontent.com/LatticeX-Foundation/Rosetta/master/example/tutorials/dsets/ALL/cls_train_x.csv
wget https://raw.githubusercontent.com/LatticeX-Foundation/Rosetta/master/example/tutorials/dsets/ALL/cls_train_y.csv

# Download test dataset.
wget https://raw.githubusercontent.com/LatticeX-Foundation/Rosetta/master/example/tutorials/dsets/ALL/cls_test_x.csv
wget https://raw.githubusercontent.com/LatticeX-Foundation/Rosetta/master/example/tutorials/dsets/ALL/cls_test_y.csv

python3 csv_utils.py cls_train_x.csv cls_train_y.csv train 
python3 csv_utils.py cls_test_x.csv cls_test_y.csv test

rm -rf cls_train_x.csv 
rm -rf cls_train_y.csv 
rm -rf cls_test_x.csv
rm -rf cls_test_y.csv
rm -rf csv_utils.py