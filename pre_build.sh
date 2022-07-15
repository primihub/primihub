#!/bin/bash

if [ "$(uname)" == "Darwin" ]; then
# Mac OS X 操作系统
  CONFIG=`python3.9-config --ldflags` && NEWLINE="[\"${CONFIG}\"] + [\"-lpython3.9\"]"
  sed -e "s|PLACEHOLDER-PYTHON3.X-CONFIG|${NEWLINE}|g" BUILD.bazel > BUILD.bazel.tmp && mv BUILD.bazel.tmp BUILD.bazel
  echo "done"
elif ["$(expr substr $(uname -s) 1 5)" == "Linux"]; then
# GNU/Linux操作系统
  PrevLineNum=`cat BUILD.bazel | grep -n "PLACEHOLDER-PYTHON3.X-CONFIG" | awk -F ":" '{print $1}'`
    if [ -z ${PrevLineNum} ]; then
    echo "Can't find line including 'PLACEHOLDER-PYTHON3.X-CONFIG' in BUILD.bazel."
    exit
  fi

  TargetLine=`expr $PrevLineNum + 3`

  CONFIG=`python3.9-config --ldflags` \
    && NEWLINE="\ \ \ \ linkopts = LINK_OPTS + [\"${CONFIG} -lpython3.9\"]," \
    && sed -i "${TargetLine}c ${NEWLINE}" BUILD.bazel

  echo "done"
elif ["$(expr substr $(uname -s) 1 10)" == "MINGW32_NT"]; then
# Windows NT操作系统
  echo "not supported"
fi