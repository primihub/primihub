#!/bin/sh

# update local python LINK_OPTS
CONFIG=`python3-config --ldflags` \
  && NEWLINE="\ \ \ \ linkopts = LINK_OPTS + [\"${CONFIG}\"]," \
  && sed -i "451c ${NEWLINE}" BUILD.bazel

echo "done"