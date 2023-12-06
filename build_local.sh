#/bin/bash
# When using this script, please make sure that the python version of the local machine is 3.8

if [ ! -n "$1" ] ; then
    TAG=`date +%F-%H-%M-%S`
else
    TAG=$1
fi

if [ ! -n "$2" ] ; then
    IMAGE_NAME="primihub/primihub-node"
else
    IMAGE_NAME=$2
fi

bash pre_build.sh

make mysql=y

if [ $? -ne 0 ]; then
    echo "Build failed!!!"
    exit
fi

git rev-parse --abbrev-ref HEAD >> commit.txt
git rev-parse HEAD >> commit.txt

tar zcfh bazel-bin.tar.gz bazel-bin/cli \
        bazel-bin/node \
        bazel-bin/_solib* \
        bazel-bin/task_main \
        bazel-bin/src/primihub/pybind_warpper/opt_paillier_c2py.so \
        bazel-bin/src/primihub/pybind_warpper/linkcontext.so \
        bazel-bin/src/primihub/task/pybind_wrapper/ph_secure_lib.so \
        python \
        config \
        example \
        data \
        commit.txt

docker build -t $IMAGE_NAME:$TAG . -f Dockerfile.local