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

FROM ubuntu:20.04 as builder

ENV LANG C.UTF-8 
ENV DEBIAN_FRONTEND=noninteractive

# Install tools
RUN apt update \
  && apt install -y python3 python3-dev python3-distutils libgmp-dev python-dev \
  && apt install -y gcc-8 automake ca-certificates git g++-8 libtool m4 patch pkg-config unzip make wget curl zip ninja-build npm \
  && update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-8 800 --slave /usr/bin/g++ g++ /usr/bin/g++-8 \
  && rm -rf /var/lib/apt/lists/*

# install cmake
RUN wget https://primihub.oss-cn-beijing.aliyuncs.com/cmake-3.20.2-linux-x86_64.tar.gz \
  && tar -zxf cmake-3.20.2-linux-x86_64.tar.gz \
  && chmod +x cmake-3.20.2-linux-x86_64/bin/cmake \
  && ln -s `pwd`/cmake-3.20.2-linux-x86_64/bin/cmake /usr/bin/cmake \
  && rm -f cmake-3.20.2-linux-x86_64.tar.gz 

# install bazelisk
RUN npm install -g @bazel/bazelisk

WORKDIR /src
ADD . /src

# Bazel build primihub-node & primihub-cli & paillier shared library
RUN bash pre_docker_build.sh \
  && bazel build --config=linux :node :cli :opt_paillier_c2py

FROM ubuntu:20.04 as runner

ENV LANG C.UTF-8
ENV DEBIAN_FRONTEND=noninteractive

RUN  apt-get update \
  && apt-get install -y python3 python3-dev libgmp-dev python3-pip git \
  && rm -rf /var/lib/apt/lists/*

ARG TARGET_PATH=/root/.cache/bazel/_bazel_root/f8087e59fd95af1ae29e8fcb7ff1a3dc/execroot/__main__/bazel-out/k8-fastbuild/bin
WORKDIR $TARGET_PATH
# Copy binaries to TARGET_PATH
COPY --from=builder $TARGET_PATH ./
# Copy test data files to /tmp/
COPY --from=builder /src/data/ /tmp/
# Make symlink to primihub-node & primihub-cli
RUN mkdir /app/config /app/primihub_python -p && ln -s $TARGET_PATH/node /app/primihub-node && ln -s $TARGET_PATH/cli /app/primihub-cli

# Change WorkDir to /app
WORKDIR /app

# Copy all test config files to /app
COPY --from=builder /src/config/ ./config/

# Copy primihub python sources to /app and setup to system python3
COPY --from=builder /src/python/ ./primihub_python/

RUN cd primihub_python \
  && python3 -m pip install --upgrade pip \
  && pip3 install -r requirements.txt \
  && python3 setup.py install

ENV PYTHONPATH=/usr/lib/python3.8/site-packages/:$TARGET_PATH

# gRPC server port
EXPOSE 50050 8888
# Cryptool port
EXPOSE 12120
EXPOSE 12121
