import grpc
import csv
import time
import express_pb2
import express_pb2_grpc
import threading
import pandas
import pybind_mpc
from concurrent import futures
from concurrent.futures import ThreadPoolExecutor
from multiprocessing import Process


class MPCExpressService(express_pb2_grpc.MPCExpressTaskServicer):
    def __init__(self):
        self.timeout_check_timer = threading.Timer(10, self.CheckTimeout)
        self.clean_timer = threading.Timer(10, self.CleanHistoryTask)

    def TaskStart(self, request, response):
        party_id = request.local_partyid
        job_id = request.jobid
        party_addr = request.addr
        expr = request.expr
        input_file_path = request.input_filepath
        output_file_path = request.output_filepath

        reveal_party = [0, 1, 2]

        col_owner = {}
        col_dtype = {}

        for col in request.columns:
            col_owner[col.name] = col.owner
            col_dtype[col.name] = col.float_type

        mpc_args = (job_id, party_id, expr, col_owner, col_dtype,
                    party_addr, input_file_path, output_file_path,
                    reveal_party)

        p = Process(target=MPCExpressService.RunMPCEvaluator, args=mpc_args) 
        p.start()
        
        pid = p.pid
        time.sleep(1)
        p.kill()


    def TaskStop(self, request, response):
        pass

    def TaskStatus(self, request, response):
        pass

    def CheckTimeout(self):
        pass

    def CleanHistoryTask(self):
        pass

    @staticmethod
    def RunMPCEvaluator(job_id, party_id, expr, col_owner,
                        col_dtype, party_addr, input_file_path,
                        output_file_path, reveal_party):
        mpc_exec=pybind_mpc.MPCExpressExecutor(party_id)
        mpc_exec.import_column_config(col_owner, col_dtype)
        mpc_exec.import_express(expr)

        df=pandas.read_csv(input_file_path)
        for col in df.columns:
            val_list=df.iloc[col]
            mpc_exec.import_column_values(col, val_list)

        mpc_exec.evaluate(party_addr[0], party_addr[1], party_addr[2])
        result=mpc_exec.reveal_mpc_result(reveal_party)
        if result:
            with open(output_file_path, "w") as f:
                writer=csv.writer(f)
                writer.writerow(expr)
                writer.writerow(result)


def serve():
    server=grpc.server(futures.ThreadPoolExecutor(max_workers=10))
    express_pb2_grpc.add_MPCExpressTaskServicer_to_server(
        MPCExpressService(), server)
    server.add_insecure_port('[::]:50051')
    server.start()
    print("Start grpc server at 50051.")
    server.wait_for_termination()


if __name__ == '__main__':
    serve()
