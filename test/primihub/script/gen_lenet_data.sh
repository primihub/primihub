set -x
set -e


#cd ../../../data/falcon/dataset/MNIST
cd /tmp

set +e
rm -rf test_data_A
rm -rf test_data_B
rm -rf test_data_C
rm -rf test_labels_A
rm -rf test_labels_B
rm -rf test_labels_C
rm -rf train_data_A
rm -rf train_data_B
rm -rf train_data_C
rm -rf train_labels_A
rm -rf train_labels_B
rm -rf train_labels_C
set -e

# Download train dataset.
wget https://primihub.oss-cn-beijing.aliyuncs.com/data/train_data_A
wget https://primihub.oss-cn-beijing.aliyuncs.com/data/train_data_B
wget https://primihub.oss-cn-beijing.aliyuncs.com/data/train_data_C
wget https://primihub.oss-cn-beijing.aliyuncs.com/data/train_labels_A
wget https://primihub.oss-cn-beijing.aliyuncs.com/data/train_labels_B
wget https://primihub.oss-cn-beijing.aliyuncs.com/data/train_labels_C


# Download test dataset.
wget https://primihub.oss-cn-beijing.aliyuncs.com/data/test_data_A
wget https://primihub.oss-cn-beijing.aliyuncs.com/data/test_data_B
wget https://primihub.oss-cn-beijing.aliyuncs.com/data/test_data_C
wget https://primihub.oss-cn-beijing.aliyuncs.com/data/test_labels_A
wget https://primihub.oss-cn-beijing.aliyuncs.com/data/test_labels_B
wget https://primihub.oss-cn-beijing.aliyuncs.com/data/test_labels_C


