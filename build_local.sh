#/bin/bash

if [ ! -n "$1" ] ; then
    echo "Please input 1st arg: docker image tag"
    exit
fi

PrevLineNum=`cat BUILD.bazel | grep -n "PLACEHOLDER-PYTHON3.X-CONFIG" | awk -F ":" '{print $1}'`
if [ -z ${PrevLineNum} ]; then
        echo "Can't find line including 'PLACEHOLDER-PYTHON3.X-CONFIG' in BUILD.bazel."
        exit
fi

TargetLine=`expr $PrevLineNum + 3`

#Please modify the following Python version according to your local actual environment
CONFIG=`python3.9-config --ldflags` \
  && NEWLINE="\ \ \ \ linkopts = LINK_OPTS + [\"${CONFIG} -lpython3.9\"]," \
  && sed -i "${TargetLine}c ${NEWLINE}" BUILD.bazel

echo "Done"

#build
#bazel build --config=linux :node :cli
bazel build --config=linux :node :cli :opt_paillier_c2py

if [ $? -ne 0 ]; then
    echo "Build failed!!!"
    exit
fi

BASE_DIR=`ls -l | grep bazel-bin | awk '{print $11}'`
IMAGE_NAME="primihub/primihub_node"

sed -i "10c ARG TARGET_PATH=$BASE_DIR" Dockerfile.local

#rm -rf $BASE_DIR/python $BASE_DIR/config
rm -f $BASE_DIR/Dockerfile.local
rm -f $BASE_DIR/.dockerignore
#rm -rf $BASE_DIR/data

cp -r ./data $BASE_DIR/
cp -r ./python $BASE_DIR/
cp -r ./config $BASE_DIR/
cp ./Dockerfile.local $BASE_DIR/
cp -r ./src $BASE_DIR/

cd $BASE_DIR
find ./ -name "_objs" > .dockerignore

docker build -t $IMAGE_NAME:$1 . -f Dockerfile.local
