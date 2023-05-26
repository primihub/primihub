#!/bin/bash
set -x
user_pids=$(ps -ef | awk '{print $1}' |grep ${USER} |grep -v grep)
if [ -z "${user_pids}" ]; then
  pids=$(ps -ef |grep "bazel-bin/node" | grep -v grep |awk '{print $2}')
else
  pids=$(ps -ef |grep "bazel-bin/node" |grep ${USER} | grep -v grep |awk '{print $2}')
fi
echo $pids
if [ -n "${pids}" ]; then
  kill -9 ${pids}
  echo "stop done!"
fi
