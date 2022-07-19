import sys
from os import path

import grpc

# sys.path.append(path.abspath(path.join(path.dirname(__file__), "ph_grpc")))  # noqa
# from src.primihub.protos import common_pb2, worker_pb2, worker_pb2_grpc  # noqa

from primihub.client import client


def test_grpc_pir():
    grpc_client = client.GRPCClient(node="192.168.99.26:8050", cert="")

    grpc_client.set_task_map(
        task_type=2,
        name="test-pri-task",
        language=3,
        # params={
        #     "outputFullFilename": "./data/result/pir.csv",
        #     "queryIndeies": "1",
        #     "serverData": "pir_server_data"
        # },
        code=b"import sys;",
        # input_datasets="serverData",
        job_id=b"666",
        task_id=b"1"
    )
    res = grpc_client.submit()
    print(res)


if __name__ == "__main__":
    test_grpc_pir()
