#/bin/bash
# When using this script, please make sure that the python version of the local machine is 3.8

if [ ! -n "$1" ] ; then
    TAG=`date +%F-%H-%M-%S`
else
    TAG=$1
fi

if [ ! -n "$2"] ; then
    IMAGE_NAME="primihub/primihub-node"
else
    IMAGE_NAME=$2
fi

bash pre_build.sh

ARCH=`arch`


bazel build --config=linux_$ARCH //:node \
    //:py_main \
    //:cli \
    //src/primihub/pybind_warpper:opt_paillier_c2py \
    //src/primihub/pybind_warpper::linkcontext

if [ $? -ne 0 ]; then
    echo "Build failed!!!"
    exit
fi

BASE_DIR=`ls -l | grep bazel-bin | awk '{print $11}'`

if [ ! -d "$BASE_DIR" ]; then
    echo "BASE_DIR IS NULL"
    exit
fi

key_word="ARG TARGET_PATH="
row_num=$(sed -n "/${key_word}/=" Dockerfile.local)
sed -i "${row_num}s#.*#ARG TARGET_PATH="${BASE_DIR}"#" Dockerfile.local

rm -rf $BASE_DIR/python $BASE_DIR/config
rm -f $BASE_DIR/Dockerfile.local
rm -f $BASE_DIR/.dockerignore
rm -rf $BASE_DIR/data

cp -r ./data $BASE_DIR/
cp -r ./python $BASE_DIR/
cp -r ./config $BASE_DIR/
cp ./Dockerfile.local $BASE_DIR/
cp -r ./src $BASE_DIR/

cd $BASE_DIR
find ./ -name "_objs" > .dockerignore

docker build -t $IMAGE_NAME:$TAG . -f Dockerfile.local