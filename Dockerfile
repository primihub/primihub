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

# Install dependencies
RUN  apt update \
  && apt install -y python3 python3-dev gcc-8 g++-8 python-dev libgmp-dev cmake libmysqlclient-dev \
  && apt install -y automake ca-certificates git libtool m4 patch pkg-config unzip make wget curl zip ninja-build npm \
  && update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-8 800 --slave /usr/bin/g++ g++ /usr/bin/g++-8 \
  && rm -rf /var/lib/apt/lists/*

# install  bazelisk
RUN npm install -g @bazel/bazelisk

WORKDIR /src
ADD . /src

# Bazel build primihub-node & primihub-cli & paillier shared library
RUN bash pre_build.sh \
  && mv -f WORKSPACE_GITHUB WORKSPACE \
  && bazel build --config=linux_`arch` //:node \
    //:py_main \
    //:cli \
    //src/primihub/pybind_warpper:opt_paillier_c2py \
    //src/primihub/pybind_warpper::linkcontext \
  && tar zcf /opt/bazel-bin.tar.gz --exclude=*_objs ./bazel-bin/*

FROM ubuntu:20.04 as runner

ENV DEBIAN_FRONTEND=noninteractive

# Install python3 and GCC openmp (Depends with cryptFlow2 library)
RUN apt-get update \
  && apt-get install -y python3 python3-dev libgmp-dev python3-pip libzmq5 tzdata libmysqlclient-dev \
  && rm -rf /var/lib/apt/lists/*

COPY --from=builder /opt/bazel-bin.tar.gz /opt/bazel-bin.tar.gz
# Copy test data files to /tmp/
COPY --from=builder /src/data/ /tmp/
# Copy all test config files to /app/config
COPY --from=builder /src/config /app/config
# Copy primihub python sources to /app and setup to system python3
COPY --from=builder /src/python /app/python
COPY --from=builder /src/src/primihub/protos/ /app/src/primihub/protos/

WORKDIR /app

RUN tar zxf /opt/bazel-bin.tar.gz \
  && mkdir log

# Copy opt_paillier_c2py.so linkcontext.so to /app/python, this enable setup.py find it.
RUN mv $TARGET_PATH/src/primihub/pybind_warpper/opt_paillier_c2py.so /app/python/ \
  && mv $TARGET_PATH/src/primihub/pybind_warpper/linkcontext.so /app/python/
  && ln -s bazel-bin/node primihub-node && ln -s bazel-bin/cli primihub-cli

WORKDIR /app/python

RUN python3 -m pip install --upgrade pip \
  && python3 -m pip install -r requirements.txt \
  && python3 setup.py install \
  && rm -rf /root/.cache/pip/

WORKDIR /app

# gRPC server port
EXPOSE 50050
