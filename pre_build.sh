#!/bin/bash
set -x
set -e

[[ -z "$1" ]] && PRIMIHUB_MODE="FULL"
[[ -n "$1" ]] && PRIMIHUB_MODE=$1

if [[ "$PRIMIHUB_MODE" == "FULL" ]]; then
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

  PYTHON_CONFIG_CMD="python$U_V1.$U_V2-config"

  if ! command -v ${PYTHON_CONFIG_CMD} >/dev/null 2>&1; then
    echo "please install python$U_V1.$U_V2-dev"
    exit
  fi

  #get python include path
  PYTHON_INC_CONFIG=`${PYTHON_CONFIG_CMD} --includes | awk '{print $1}' |awk -F'-I' '{print $2}'`
  if [ ! -d "${PYTHON_INC_CONFIG}" ]; then
    echo "${PYTHON_CONFIG_CMD} get python include path failed"
    exit -1
  fi

  # link python include path into workspace
  pushd third_party
  rm -f python_headers
  ln -s ${PYTHON_INC_CONFIG} python_headers
  popd

  #get python install prefix
  PREFIX_PATH=`${PYTHON_CONFIG_CMD} --prefix`

  #get python link option
  CONFIG=`${PYTHON_CONFIG_CMD} --ldflags` && {
    NEWLINE="[\"-L${PREFIX_PATH}/lib ${CONFIG}\"] + [\"-lpython$U_V1.$U_V2\"]"
  }

  # Compatible with MacOS
  sed -e "s|LINK_PYTHON_OPTS =.*$|LINK_PYTHON_OPTS = ${NEWLINE}|g" BUILD.bazel > BUILD.bazel.tmp \
      && mv BUILD.bazel.tmp BUILD.bazel
else
  # sed -e "s|PLACEHOLDER-PYTHON3.X-CONFIG|[]|g" BUILD.bazel > BUILD.bazel.tmp && mv BUILD.bazel.tmp BUILD.bazel
  sed -e "s|LINK_PYTHON_OPTS =.*$|LINK_PYTHON_OPTS = []|g" BUILD.bazel > BUILD.bazel.tmp \
      && mv BUILD.bazel.tmp BUILD.bazel

fi
echo "done"

#detect platform and machine hardware
KERNEL_NAME=$(uname -s)
KERNEL_NAME=$(echo $KERNEL_NAME | tr '[:upper:]' '[:lower:]')
if [[ "$PRIMIHUB_MODE" == "FULL" ]]; then
if [ "${KERNEL_NAME}" == "linux" ]; then
if ! command -v chrpath > /dev/null 2>&1; then
  echo "chrpath command is not availe"
  echo "please install apt-get install chrpath for ubuntu, or yum install chrpath for centos"
  exit -1
fi
fi
fi
MACHINE_HARDWARE=$(uname -m)
sed -e "s|PLATFORM_HARDWARE|${KERNEL_NAME}_${MACHINE_HARDWARE}|g" Makefile > Makefile.tmp && mv Makefile.tmp Makefile
