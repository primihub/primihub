#/bin/bash
# When using this script, please make sure that the python version of the local machine is 3.8
set -x
mode_full="FULL"

[[ -z "$1" ]] && PRIMIHUB_MODE=$mode_full
[[ -n "$1" ]] && PRIMIHUB_MODE=$1

if [[ "$PRIMIHUB_MODE" != "${mode_full}" ]]; then
  PRIMIHUB_MODE="MINI"
fi

if [ ! -n "$2" ] ; then
    TAG=`date +%F-%H-%M-%S`
else
    TAG=$2
fi

if [ ! -n "$3" ] ; then
    IMAGE_NAME="primihub/primihub-node"
else
    IMAGE_NAME=$3
fi

bash pre_build.sh "$PRIMIHUB_MODE"

build_opt="mysql=y"
if [[ "$PRIMIHUB_MODE" == "FULL" ]]; then
  build_opt="${build_opt}"
else
  build_opt="${build_opt} disable_py_task=y"
fi

make $build_opt

if [ $? -ne 0 ]; then
    echo "Build failed!!!"
    exit
fi

git rev-parse --abbrev-ref HEAD >> commit.txt
git rev-parse HEAD >> commit.txt
release_pkg="bazel-bin/cli \
  bazel-bin/node \
  bazel-bin/_solib* \
  bazel-bin/task_main \
  config \
  example \
  data \
  commit.txt \
"

if [[ "$PRIMIHUB_MODE" == "FULL" ]]; then
release_pkg="${release_pkg} \
  bazel-bin/src/primihub/pybind_warpper/opt_paillier_c2py.so \
  bazel-bin/src/primihub/pybind_warpper/linkcontext.so \
  bazel-bin/src/primihub/task/pybind_wrapper/ph_secure_lib.so \
  python \
"
fi

tar zcfh bazel-bin.tar.gz ${release_pkg}

if [[ "$PRIMIHUB_MODE" == "FULL" ]]; then
  docker_file=Dockerfile.local
  docker build -t $IMAGE_NAME:$TAG . -f ${docker_file}
else
  docker_file=Dockerfile.mini
  docker build -t $IMAGE_NAME:mini-$TAG . -f ${docker_file}
fi
