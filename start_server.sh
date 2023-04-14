#!/bin/bash
file_name=$0
file_dir=$(dirname $(readlink -f ${file_name}))
echo ${file_dir}
py_primihub_path="${file_dir}/python"
if [ ! -d "${py_primihub_path}" ]; then
  echo "${py_primihub_path} is not exists, please check ..."
  exit -1
fi
export PYTHONPATH=${PYTHONPATH}:${py_primihub_path}
if [ ! -d localdb ]; then
  mkdir localdb
fi
# log_level 1->7,the larger the value the more detailed the log
log_level=7
export GLOG_logtostderr=1 GLOG_v=${log_level} 
nohup ./bazel-bin/node --node_id=node0 --config=./config/node0.yaml >> log_node0 2>&1 &
nohup ./bazel-bin/node --node_id=node1 --config=./config/node1.yaml >> log_node1 2>&1 &
nohup ./bazel-bin/node --node_id=node2 --config=./config/node2.yaml >> log_node2 2>&1 &
