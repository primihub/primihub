#/bin/bash

if [ ! -n "$1" ] ; then
    echo "Please input 1st arg: docker image tag"
    exit
fi

bash pre_build.sh

#build
bazel build --config=linux :node :cli :opt_paillier_c2py

if [ $? -ne 0 ]; then
    echo "Build failed!!!"
    exit
fi

BASE_DIR=`ls -l | grep bazel-bin | awk '{print $11}'`

if [ !$BASE_DIR ]; then
    echo "BASE_DIR IS NULL"
    exit
fi

IMAGE_NAME="primihub/primihub-node"

sed -i "12c ARG TARGET_PATH=$BASE_DIR" Dockerfile.local

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

docker build -t $IMAGE_NAME:$1 . -f Dockerfile.local 