#!/bin/bash

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
