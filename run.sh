#!/bin/bash
GRPC_TRACE=all GLOG_logtostderr=1 GLOG_v=7 ./bazel-bin/node --node_id=node0 --service_port=50050 --config=./config/node0.yaml &> log_node0 &
GRPC_TRACE=all GLOG_logtostderr=1 GLOG_v=7 ./bazel-bin/node --node_id=node1 --service_port=50051 --config=./config/node1.yaml &> log_node1 &
GRPC_TRACE=all GLOG_logtostderr=1 GLOG_v=7 ./bazel-bin/node --node_id=node2 --service_port=50052 --config=./config/node2.yaml &> log_node2 &
