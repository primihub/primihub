#!/bin/bash
set -x
if [ -z "${USER}" ]; then
  pids=$(ps -ef |grep "bazel-bin/node" | grep -v grep |awk '{print $2}')
else
  pids=$(ps -ef |grep "bazel-bin/node" |grep ${USER} | grep -v grep |awk '{print $2}')
fi
echo $pids
if [ -n "${pids}" ]; then
  kill -9 ${pids}
  echo "stop done!"
fi
