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
if command -v yum &>/dev/null; then
CONFIG=`python$U_V1.$U_V2-config --ldflags` && NEWLINE="[\"${CONFIG}\"]"
else
CONFIG=`python$U_V1.$U_V2-config --ldflags` && NEWLINE="[\"${CONFIG}\"] + [\"-lpython$U_V1.$U_V2\"]"
fi
PYTHON_INCLUDES=`python$U_V1.$U_V2-config --includes` && INCLUDE_PATH="[\"${PYTHON_INCLUDES}\"]"
# Compatible with MacOS
sed -e "s|PLACEHOLDER-PYTHON3.X-CONFIG|${NEWLINE}|g" BUILD.bazel > BUILD.bazel.tmp && mv BUILD.bazel.tmp BUILD.bazel
sed -e "s|PLACEHOLDER-PYTHON3.X-INCLUDES|${INCLUDE_PATH}|g" BUILD.bazel > BUILD.bazel.tmp && mv BUILD.bazel.tmp BUILD.bazel
echo "done"
pushd src/primihub/pybind_warpper
sed -e "s|PLACEHOLDER-PYTHON3.X-CONFIG|${NEWLINE}|g" BUILD > BUILD.bazel.tmp && mv BUILD.bazel.tmp BUILD
sed -e "s|PLACEHOLDER-PYTHON3.X-INCLUDES|${INCLUDE_PATH}|g" BUILD > BUILD.bazel.tmp && mv BUILD.bazel.tmp BUILD
popd
if [ ! -d "localdb" ]; then
    mkdir localdb
fi

if [ ! -d "log" ]; then
    mkdir log
fi
