#!/bin/bash
SERVER_INFO="127.0.0.1:50050"
if [ ! -d example ]; then
  echo "no example found"
  exit -1
fi
#case_path="example/FL/logistic_regression"
#case_path="example/FL/xgboost"
case_path="example"
case_list=$(ls ${case_path})
for case_info in ${case_list[@]}; do
  [ -d "example/${case_info}" ] && continue
  echo "./bazel-bin/cli --server=${SERVER_INFO} --task_config_file=${case_path}/${case_info}"
  ./bazel-bin/cli --server="${SERVER_INFO}" --task_config_file="${case_path}/${case_info}"
done
