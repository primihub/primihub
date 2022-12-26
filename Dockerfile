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


FROM primihub/primihub-node:build as builder

WORKDIR /src
ADD . /src

# Bazel build primihub-node & primihub-cli & paillier shared library
RUN bash pre_build.sh \
  && bazel build --config=linux --define cpu=amd64 --define microsoft-apsi=true :node :cli :opt_paillier_c2py

FROM ubuntu:20.04 as runner

ENV DEBIAN_FRONTEND=noninteractive

# Install python3 and GCC openmp (Depends with cryptFlow2 library)
RUN apt-get update \
  && apt-get install -y python3 python3-dev libgmp-dev python3-pip libzmq5 tzdata \
  && rm -rf /var/lib/apt/lists/*

ARG TARGET_PATH=/root/.cache/bazel/_bazel_root/f8087e59fd95af1ae29e8fcb7ff1a3dc/execroot/primihub/bazel-out/k8-fastbuild/bin
WORKDIR $TARGET_PATH

# Copy binaries to TARGET_PATH
COPY --from=builder $TARGET_PATH ./
# Copy test data files to /tmp/
COPY --from=builder /src/data/ /tmp/

# Change WorkDir to /app
WORKDIR /app

# Make symlink to primihub-node & primihub-cli
RUN ln -s $TARGET_PATH/node /app/primihub-node && ln -s $TARGET_PATH/cli /app/primihub-cli

# Copy all test config files to /app/config
COPY --from=builder /src/config ./config

# Copy primihub python sources to /app and setup to system python3
RUN mkdir -p src/primihub/protos data log
COPY --from=builder /src/python ./python
COPY --from=builder /src/src/primihub/protos/ ./src/primihub/protos/
RUN cp $TARGET_PATH/opt_paillier_c2py.so /app/python/

WORKDIR /app/python
RUN python3 -m pip install --upgrade pip \
  && python3 -m pip install -r requirements.txt \
  && python3 setup.py install

RUN rm -rf /app/python/opt_paillier_c2py.so
WORKDIR /app

# gRPC server port
EXPOSE 50050
# Cryptool port
EXPOSE 12120
EXPOSE 12121
