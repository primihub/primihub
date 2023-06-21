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

ARCH=`arch`

bazel build --config=linux_$ARCH --define enable_mysql_driver=true //:node \
    //:py_main \
    //:cli \
    //src/primihub/pybind_warpper:opt_paillier_c2py \
    //src/primihub/pybind_warpper:linkcontext

if [ $? -ne 0 ]; then
    echo "Build failed!!!"
    exit
fi

tar zcf primihub-linux-amd64.tar.gz bazel-bin/cli \
             bazel-bin/node \
             bazel-bin/py_main \
             bazel-bin/src/primihub/pybind_warpper/opt_paillier_c2py.so \
             bazel-bin/src/primihub/pybind_warpper/linkcontext.so \
             python \
             config \
             example \
             data

docker build -t $IMAGE_NAME:$TAG . -f Dockerfile.local