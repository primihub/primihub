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


def run_grpc_client():
    conn = grpc.insecure_channel("192.168.99.22:50051")
    stub = express_pb2_grpc.MPCExpressTaskStub(channel=conn)

    # Start new task.
    request = express_pb2.MPCExpressRequest()

    request.jobid = "jobid"
    request.local_partyid = 0

    request.columns.append(express_pb2.PartyColumn(
        name="A", owner=0, float_type=True))
    request.columns.append(express_pb2.PartyColumn(
        name="B", owner=1, float_type=True))
    request.columns.append(express_pb2.PartyColumn(
        name="C", owner=2, float_type=True))
    request.columns.append(express_pb2.PartyColumn(
        name="D", owner=2, float_type=True))

    request.output_filepath = "/tmp/test.csv"
    request.input_filepath = "/home/zxy/expr/party_0_data.csv"

    request.expr = "A+B*C-D"

    addr = request.addr
    addr.ip_prev = "127.0.0.1"
    addr.ip_next = "127.0.0.1"
    addr.port_next = 10070
    addr.port_prev = 10070

    response = stub.TaskStart(request)
    print(response)

    # Stop task created just now.
    time.sleep(5)

    stop_request = express_pb2.MPCExpressRequest()
    stop_request.jobid = "jobid"

    response = stub.TaskStop(stop_request)
    print(response)


class MYSQLOperator():
    def __init__(self):
        self.conn=pymysql.connect(
            host="127.0.0.1",
            port=3306,
            database="Primihub",
            charset="utf8",
            user="root",
            passwd="123456"
        )
    def close_conn():
        conn.close()

    def TaskStart(self, jobid, pid):
        #insert one record,get jobid,pid,status,`errmsg` ,
        status="running"
        errmsg=""
        cursor = conn.cursor()
        #insert
        try:
            sql = 'INSERT INTO task(jobid,pid,status,errmsg) VALUES(%s,%s,%s,%s);'
            cursor.execute(sql,(jobid,pid,status,errmsg))
            conn.commit()
            print("insert success!")
        except Exception as e:
            conn.rollback()
            print("error:\n",e)

    def TaskStop(self, jobid):
        errmsg=""
        status="cancelled"
        cursor = conn.cursor()
        try:
            #inquiry pid by jobid
            sql = 'SELECT * FROM task where jobid=%s;'
            cursor.execute(sql,jobid)
            pid = cursor.fetchone()[1]
            #kill process by pid
            os.kill(int(pid))
            #modified stauts as cancelled
            sql = "UPDATE task SET status = %s WHERE jobid = %s;"
            cursor.execute(sql,(status,jobid))
            conn.commit()
        except Exception as e:
            conn.rollback()
            print("error:\n",e)

    def TaskStatus(self, jobid):
        cursor = conn.cursor()
        try:
            sql = 'SELECT * FROM task where jobid=%s;'
            cursor.execute(sql,jobid)
            status = cursor.fetchone()[2]
            return status
        except Exception as e:
            print("error:\n",e)

    def CheckTimeout(self):
        pass

    def CleanHistoryTask(self):
        cursor = conn.cursor()
        try:
            sql = 'DELETE FROM task where start_time<DATE_ADD(CURRENT_TIMESTAMP(), INTERVAL -30 DAY);'
            affect_rows=cursor.execute(sql)
            print(affect_rows)
        except Exception as e:
            print("error:\n",e)

class MPCExpressService(express_pb2_grpc.MPCExpressTaskServicer):
    jobid_pid_map = {}

    def __init__(self):
        self.timeout_check_timer = threading.Timer(10, self.CheckTimeout)
        self.clean_timer = threading.Timer(10, self.CleanHistoryTask)

    def TaskStart(self, request, context):
        party_id = request.local_partyid
        job_id = request.jobid
        party_addr = (request.addr.ip_next, request.addr.ip_prev,
                      request.addr.port_next, request.addr.port_prev)
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
        p.daemon = True
        p.start()

        MPCExpressService.jobid_pid_map[job_id] = p

        response = express_pb2.MPCExpressResponse()
        response.jobid = request.jobid
        response.status = express_pb2.TaskStatus.TASK_RUNNING
        response.message = "New pid is {}".format(p.pid)

        return response

    def TaskStop(self, request, context):
        jobid = request.jobid
        proc = MPCExpressService.jobid_pid_map.get(jobid, None)
        if proc is None:
            response = express_pb2.MPCExpressResponse()
            response.jobid = request.jobid
            response.message = "Can't find subprocess with jobid {}".format(
                jobid)
            return response
        else:
            response = express_pb2.MPCExpressResponse()
            if proc.is_alive():
                proc.kill()
                response.message = "Subprocess for jobid {} quit now.".format(
                    jobid)
            else:
                response.message = "Subprocess for jobid {} quit before.".format(
                    jobid)

            response.jobid = request.jobid
            return response

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
        mpc_exec = pybind_mpc.MPCExpressExecutor(party_id)
        mpc_exec.import_column_config(col_owner, col_dtype)
        mpc_exec.import_express(expr)

        df = pandas.read_csv(input_file_path)
        for col in df.columns:
            val_list = df[col].tolist()
            mpc_exec.import_column_values(col, val_list)

        mpc_exec.evaluate(party_addr[0], party_addr[1],
                          party_addr[2], party_addr[3])
        result = mpc_exec.reveal_mpc_result(reveal_party)
        if result:
            with open(output_file_path, "w") as f:
                writer = csv.writer(f)
                writer.writerow(expr)
                writer.writerow(result)


def serve():
    server = grpc.server(futures.ThreadPoolExecutor(max_workers=10))
    express_pb2_grpc.add_MPCExpressTaskServicer_to_server(
        MPCExpressService(), server)
    server.add_insecure_port('[::]:50051')
    server.start()
    print("Start grpc server at 50051.")
    server.wait_for_termination()


if __name__ == '__main__':
    serve()
