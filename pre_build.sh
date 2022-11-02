#!/bin/bash
set -x
PYTHON_BIN=python3
if ! command -v python3 >/dev/null 2>&1; then
  if ! command -v python >/dev/null 2>&1; then
    echo "please install python3"
    exit
  else
    PYTHON_BIN=python
  fi
fi
U_V1=`$PYTHON_BIN -V 2>&1|awk '{print $2}'|awk -F '.' '{print $1}'`
U_V2=`$PYTHON_BIN -V 2>&1|awk '{print $2}'|awk -F '.' '{print $2}'`
U_V3=`$PYTHON_BIN -V 2>&1|awk '{print $2}'|awk -F '.' '{print $3}'`

echo your python version is : "$U_V1.$U_V2.$U_V3"
if ! [ "${U_V1}" = 3 ] && [ "${U_V2}" > 6 ]; then
  echo "python version must > 3.6"
  exit
fi

if ! command -v python$U_V1.$U_V2-config >/dev/null 2>&1; then
  echo "please install python$U_V1.$U_V2-dev"
  exit
fi
#for mq
#check operating system
os_name=$(uname -s)
if [[ "$os_name" == "Darwin" ]]; then
zmq_lib_path=$(find /opt/ -name libzmq.a |head -n 1)
zmq_lib_path=$(echo ${zmq_lib_path}|awk -F'libzmq.a' '{print $1}')
else
zmq_lib_path=$(find /usr/local/ -name libzmq.a |head -n 1)
zmq_lib_path=$(echo ${zmq_lib_path}|awk -F'libzmq.a' '{print $1}')
fi
CONFIG=`python$U_V1.$U_V2-config --ldflags` && NEWLINE="[\"${CONFIG}\"] + [\"-lpython$U_V1.$U_V2\"]"
if [[ -n "${zmq_lib_path}" ]]; then
  NEWLINE="${NEWLINE} + [\"-L${zmq_lib_path}\"] + [\"-lzmq\"]"
fi
# Compatible with MacOS
sed -e "s|PLACEHOLDER-PYTHON3.X-CONFIG|${NEWLINE}|g" BUILD.bazel > BUILD.bazel.tmp && mv BUILD.bazel.tmp BUILD.bazel
echo "done"

if [ ! -d "localdb" ]; then
    mkdir localdb
fi

if [ ! -d "log" ]; then
    mkdir log
fi