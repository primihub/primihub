# Copyright 2022 Primihub
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.


FROM ubuntu:18.04 as builder

ENV LANG C.UTF-8
ENV DEBIAN_FRONTEND=noninteractive

# install dependencies
RUN  apt-get update \
  && apt-get install -y gcc-8 automake ca-certificates git g++-8 libtool m4 patch pkg-config python3 python3-dev python-dev unzip make wget curl zip ninja-build \
  && update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-8 800 --slave /usr/bin/g++ g++ /usr/bin/g++-8 

# install npm 
RUN apt-get install -y npm

# install cmake
RUN wget https://primihub.oss-cn-beijing.aliyuncs.com/cmake-3.20.2-linux-x86_64.tar.gz \
  && tar -zxf cmake-3.20.2-linux-x86_64.tar.gz \
  && chmod +x cmake-3.20.2-linux-x86_64/bin/cmake \
  && ln -s `pwd`/cmake-3.20.2-linux-x86_64/bin/cmake /usr/bin/cmake \
  && rm -rf /var/lib/apt/lists/* cmake-3.20.2-linux-x86_64.tar.gz 

# install bazelisk
RUN npm install -g @bazel/bazelisk

WORKDIR /src
ADD . /src

# generate test data files
RUN cd test/primihub/script/ \
  && /bin/bash gen_logistic_data.sh

# Update pybind11 link options in BUILD.bazel
RUN CONFIG=`python3-config --ldflags` \
  && NEWLINE="\ \ \ \ linkopts = LINK_OPTS + [\"${CONFIG}\"]," \
  && sed -i "354c ${NEWLINE}" BUILD.bazel

# Bazel build primihub-node & primihub-cli
RUN bazel build --config=linux :node :cli

FROM ubuntu:18.04 as runner

RUN  apt-get update \
  && apt-get install -y python3 python3-dev \
  && rm -rf /var/lib/apt/lists/*

ARG TARGET_PATH=/root/.cache/bazel/_bazel_root/f8087e59fd95af1ae29e8fcb7ff1a3dc/execroot/__main__/bazel-out/k8-fastbuild/bin
WORKDIR $TARGET_PATH
# Copy binaries to TARGET_PATH
COPY --from=builder $TARGET_PATH ./
# Copy test data files to /tmp/
COPY --from=builder /tmp/ /tmp/
# Make symlink to primihub-node & primihub-cli
RUN mkdir /app && ln -s $TARGET_PATH/node /app/primihub-node && ln -s $TARGET_PATH/cli /app/primihub-cli

# Change WorkDir to /app
WORKDIR /app
# Copy all test config files to /app
COPY --from=builder /src/config ./

# gRPC server port
EXPOSE 50050
# Cryptool port
EXPOSE 12120
EXPOSE 12121





