#!/bin/bash
set -x
pids=$(ps -ef |grep "bazel-bin/node" |grep -v grep |awk '{print $2}')
echo $pids
if [[ -n "${pids}" ]]; then
  kill -9 ${pids}
fi
