## Algorithm
### 1.Logistic Regression
A implement of LR based on aby3 protocol, and `test/primihub/algorithm/logistic_test.cc` contain a test case for it.
#### 1.1 Download dataset
```
cd test/primihub/script
sh gen_logistic_data.sh
cd ../../..
```
#### 1.2 Build for linux
```
bazel build --config=linux :algorithm_test
```
#### 1.3 Run
```
./bazel-bin/algorithm_test
```
