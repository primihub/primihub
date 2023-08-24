#!/bin/bash
user_pids=$(ps -ef | awk '{print $1}' |grep ${USER} |grep -v grep)
has_user_name=1
if [ -z "${user_pids}" ]; then
  has_user_name=0
fi

function kill_app() {
  app_name=$1
  if [ -z "${app_name}" ]; then
    echo "app is empty ${app_name}"
    return -1
  fi
  if [ "${has_user_name}" == "0" ]; then
    pids=$(ps -ef |grep "${app_name}" | grep -v grep |awk '{print $2}')
  else
    pids=$(ps -ef |grep "${app_name}" |grep ${USER} | grep -v grep |awk '{print $2}')
  fi
  echo $pids
  if [ -n "${pids}" ]; then
    kill -9 ${pids}
    echo "stop ${app_name} done!"
  fi
  return 0
}

function main() {
  #kill meta server
  meta_app="fusion-simple.jar"
  echo "begin to stop meta service"
  kill_app ${meta_app}
  echo "stop meta service done!"
  
  #kill primihub server
  primihub_app="task_main"
  echo "begin to stop subprocess primihub service"
  kill_app ${primihub_app}
  echo "stop ${primihub_app} done!"

  primihub_app="bazel-bin/node"
  echo "begin to stop primihub service"
  kill_app ${primihub_app}
  echo "stop ${primihub_app} done!"
}

main
