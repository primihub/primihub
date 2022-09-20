import express_pb2
import express_pb2_grpc
import grpc
import threading
from concurrent import futures
from concurrent.futures import ThreadPoolExecutor


class MPCExpressService(express_pb2_grpc.MPCExpressTaskServicer):
    def __init__(self):
        self.timeout_check_timer = threading.Timer(10, self.CheckTimeout)
        self.clean_timer = threading.Timer(10, self.CleanHistoryTask)

    def TaskStart(self, request, response):
        pass

    def TaskStop(self, request, response):
        pass

    def TaskStatus(self, request, response):
        pass

    def CheckTimeout(self):
        pass

    def CleanHistoryTask(self):
        pass


def serve():
    server = grpc.server(futures.ThreadPoolExecutor(max_workers=10))
    express_pb2_grpc.add_MPCExpressTaskServicer_to_server(
        MPCExpressService(), server)
    server.add_insecure_port('[::]:50051')
    server.start()
    server.wait_for_termination()


if __name__ == '__main__':
    serve()
