import grpc
import csv
import time
import threading
import pandas
import pybind_mpc
import configparser
import string
import pymysql
import express_pb2
import express_pb2_grpc
from concurrent import futures
from concurrent.futures import ThreadPoolExecutor
from multiprocessing import Process


class MPCExpressRequestGenerator:
    def __init__(self):
        self.column_attr = {}

    def set_party_id(self, party_id):
        self.party_id = party_id

    def set_job_id(self, job_id):
        self.job_id = job_id

    def set_mpc_addr(self, ip_next, ip_prev, port_next, port_prev):
        self.mpc_addr = (ip_next, ip_prev, port_next, port_prev)

    def set_input_output(self, input_path, output_path):
        self.input_path = input_path
        self.output_path = output_path

    def set_expr(self, expr):
        self.expr = expr

    def set_column_attr(self, name, owner, is_float):
        self.column_attr[name] = (owner, is_float)

    def gen_request(self):
        request = express_pb2.MPCExpressRequest()
        request.jobid = self.job_id
        request.expr = self.expr
        request.output_filepath = self.output_path
        request.input_filepath = self.input_path
        request.local_partyid = self.party_id

        for col_name, col_attr in self.column_attr.items():
            request.columns.append(express_pb2.PartyColumn(
                name=col_name, owner=col_attr[0], float_type=col_attr[1]))

        addr = request.addr
        addr.ip_next = self.mpc_addr[0]
        addr.ip_prev = self.mpc_addr[1]
        addr.port_next = self.mpc_addr[2]
        addr.port_prev = self.mpc_addr[3]

        return request 


class MPCExpressServiceClient:
    @staticmethod
    def start_task(remote_addr: string, msg: express_pb2.MPCExpressRequest):
        conn = grpc.insecure_channel(remote_addr)
        stub = express_pb2_grpc.MPCExpressTaskStub(channel=conn)
        response = stub.TaskStart(msg)
        return response

    @staticmethod
    def stop_task(remote_addr: string, job_id: string):
        request = express_pb2.MPCExpressRequest()
        request.jobid = job_id
        conn = grpc.insecure_channel(remote_addr)
        stub = express_pb2_grpc.MPCExpressTaskStub(channel=conn)
        response = stub.TaskStop(request)
        return response


def stop_mpc_task():
    jobid = "202002280915402308774"
    print(MPCExpressServiceClient.stop_task("192.168.99.21:50051", jobid))
    print(MPCExpressServiceClient.stop_task("192.168.99.22:50051", jobid))
    print(MPCExpressServiceClient.stop_task("192.168.99.26:50051", jobid))

def submit_mpc_task():
    generator = MPCExpressRequestGenerator()

    jobid = "202002280915402308774"
    filename = "/tmp/mpc_{}_result.csv".format(jobid)

    # Generate request for party 0.
    generator.set_party_id(0)
    generator.set_job_id(jobid)
    generator.set_mpc_addr("192.168.99.21", "192.168.99.21", 10020, 10030)
    generator.set_input_output("/home/primihub/expr/party_0.csv", filename)
    generator.set_expr("A+B*C+D")

    generator.set_column_attr("A", 0, True)
    generator.set_column_attr("B", 1, True)
    generator.set_column_attr("C", 2, True)
    generator.set_column_attr("D", 2, True)

    party_0_request = generator.gen_request()

    # Generate request for party 1.
    generator.set_party_id(1)
    generator.set_mpc_addr("192.168.99.22", "192.168.99.21", 10040, 10020)
    generator.set_input_output("/home/primihub/expr/party_1.csv", filename)

    party_1_request = generator.gen_request()

    # Generate request for party 2.
    generator.set_party_id(2)
    generator.set_mpc_addr("192.168.99.21", "192.168.99.22", 10050, 10060)
    generator.set_input_output("/home/primihub/expr/party_2.csv", filename)

    party_2_request = generator.gen_request()

    # Send request to service.
    party_0_response = MPCExpressServiceClient.start_task(
        "192.168.99.21:50051", party_0_request)
    party_1_response = MPCExpressServiceClient.start_task(
        "192.168.99.22:50051", party_1_request)
    party_2_response = MPCExpressServiceClient.start_task(
        "192.168.99.26:50051", party_2_request)

    print(party_0_response)
    print(party_1_response)
    print(party_2_response)


class MYSQLOperator:
    def __init__(self):
        db_config = configparser.ConfigParser()
        db_config.read_file(
            open('./dbUntils/dbMysqlConfig.cnf', encoding='utf-8', mode='rt'))

        self.conn = pymysql.connect(
            host=db_config.get('dbMysql', 'host'),
            port=int(db_config.get('dbMysql', 'port')),
            database=db_config.get('dbMysql', 'db_name'),
            charset=db_config.get('dbMysql', 'charset'),
            user=db_config.get('dbMysql', 'user'),
            passwd=db_config.get('dbMysql', 'password')
        )

    def ConnClose(self):
        self.conn.close()

    def TaskStart(self, jobid, pid):
        # insert one record,get jobid,pid,status,`errmsg` ,
        status = "running"
        errmsg = ""
        cursor = self.conn.cursor()
        # insert
        try:
            sql = 'INSERT INTO mpc_task(jobid,pid,status,errmsg) VALUES(%s,%s,%s,%s);'
            cursor.execute(sql, (jobid, pid, status, errmsg))
            self.conn.commit()
            print("insert success!")
        except Exception as e:
            self.conn.rollback()
            print("error:\n", e)

    def TaskStop(self, jobid):
        errmsg = ""
        status = "cancelled"
        cursor = self.conn.cursor()
        try:
            sql = "UPDATE mpc_task SET status = %s WHERE jobid = %s;"
            cursor.execute(sql, (status, jobid))
            self.conn.commit()
        except Exception as e:
            self.conn.rollback()
            print("error:\n", e)

    def TaskStatus(self, jobid):
        cursor = self.conn.cursor()
        try:
            sql = 'SELECT * FROM mpc_task where jobid=%s;'
            cursor.execute(sql, jobid)
            status = cursor.fetchone()[2]
            return status
        except Exception as e:
            print("error:\n", e)

    def CheckTimeout(self):
        # select status is running and timeouted
        pass

    def CleanHistoryTask(self):
        cursor = self.conn.cursor()
        try:
            sql = 'DELETE FROM mpc_task where start_time<DATE_ADD(CURRENT_TIMESTAMP(), INTERVAL -30 DAY);'
            affect_rows = cursor.execute(sql)
            print(affect_rows)
        except Exception as e:
            print("error:\n", e)


class MPCExpressService(express_pb2_grpc.MPCExpressTaskServicer):
    jobid_pid_map = {}

    def __init__(self):
        self.timeout_check_timer = threading.Timer(10, self.CheckTimeout)
        self.clean_timer = threading.Timer(10, self.CleanHistoryTask)
        self.mysql_op = MYSQLOperator()

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
        self.mysql_op.TaskStart(job_id, p.pid)
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
            self.mysql_op.TaskStop(jobid)
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
                writer.writerow([expr])
                for v in result:
                    writer.writerow([v])
            f.close()

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
