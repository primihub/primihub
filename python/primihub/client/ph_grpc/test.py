#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# test.py
# @Author :  ()
# @Link   :
# @Date   : 6/20/2022, 6:09:53 PM


from __future__ import print_function

import logging

import grpc

from src.primihub.protos import common_pb2, worker_pb2, worker_pb2_grpc

params = {
    "mBatchSize": 128,

}

code = f = open(r"disxgb.py", "rb")
task_map = {
    "type": 0,
    "language": common_pb2.Language.PYTHON,
    "code": f.read()
}


def submit_task(stub):
    request = worker_pb2.PushTaskRequest(
        intended_worker_id=b'1',
        task=task_map
    )

    print(request)
    try:
        res = stub.SubmitTask(request)
        print(res)
        return res

    except Exception as e:
        print(e)


def run():
    # NOTE(gRPC Python Team): .close() is possible on a channel and should be
    # used in circumstances in which the with statement does not fit the needs
    # of the code.
    with grpc.insecure_channel('localhost:50051') as channel:
        # stub = route_guide_pb2_grpc.RouteGuideStub(channel)
        stub = worker_pb2_grpc.VMNodeStub(channel)
        print(channel)
        paramValue = common_pb2.ParamValue()
        param = common_pb2.Params()
        print(param, paramValue)
        print(task_map)
        print("-------------- submit task --------------")
        submit_task(stub)


if __name__ == '__main__':
    logging.basicConfig()
    run()
