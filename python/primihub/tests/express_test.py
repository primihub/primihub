import pybind_mpc
import random
import multiprocessing
import csv
from primihub.MPC import express


def run_mpc_party(party_id, expr, col_owner, col_dtype,
                  col_val, party_addr, reveal_party):
    mpc_exec = pybind_mpc.MPCExpressExecutor(party_id)
    mpc_exec.import_column_config(col_owner, col_dtype)
    mpc_exec.import_express(expr)

    for name, val in col_val.items():
        mpc_exec.import_column_values(name, val)

    mpc_exec.evaluate(party_addr[0], party_addr[1],
                      party_addr[2], party_addr[3])
    result = mpc_exec.reveal_mpc_result(reveal_party)

    return mpc_exec, result


def mpc_run_without_grpc():
    reveal_party = [0, 1, 2]
    expr = "A+B*C+D"
    # expr = "B*C"

    col_owner = {"A": 0, "B": 1, "C": 2, "D": 2}
    # col_dtype = {"A": 0, "B": 0, "C": 0, "D": 0}
    col_dtype = {"A": 1, "B": 1, "C": 1, "D": 1}

    col_A = [random.uniform(1, 10000) for i in range(10000)]
    col_B = [random.uniform(1, 10000) for i in range(10000)]
    col_C = [random.uniform(1, 10000) for i in range(10000)]
    col_D = [random.uniform(1, 10000) for i in range(10000)]

    # col_A = [random.randint(1, 10000) for i in range(1000)]
    # col_B = [random.randint(1, 10000) for i in range(1000)]
    # col_C = [random.randint(1, 10000) for i in range(1000)]
    # col_D = [random.randint(1, 10000) for i in range(1000)]
    # col_A = [i+1.0  for i in range(10)]
    # col_B = [i+1.0  for i in range(10)]
    # col_C = [i+1.0  for i in range(10)]
    # col_D = [i+1.0  for i in range(10)]
    party_0_cols = {"A": col_A}
    party_1_cols = {"B": col_B}
    party_2_cols = {"C": col_C, "D": col_D}

    # Start party 1.
    party_1_addr = ("127.0.0.1", "127.0.0.1", 10030, 10010)
    party_1_args = (1, expr, col_owner, col_dtype,
                    party_1_cols, party_1_addr, reveal_party)

    party_1_proc = multiprocessing.Process(
        target=run_mpc_party, args=party_1_args)

    party_1_proc.daemon = True
    party_1_proc.start()

    # Start party 2.
    party_2_addr = ("127.0.0.1", "127.0.0.1", 10020, 10030)
    party_2_args = (2, expr, col_owner, col_dtype,
                    party_2_cols, party_2_addr, reveal_party)

    party_2_proc = multiprocessing.Process(
        target=run_mpc_party, args=party_2_args)

    party_2_proc.daemon = True
    party_2_proc.start()

    # Start party 0.
    party_0_addr = ("127.0.0.1", "127.0.0.1", 10010, 10020)
    res = run_mpc_party(0, expr, col_owner, col_dtype,
                        party_0_cols, party_0_addr, reveal_party)

    mpc_exec = res[0]
    mpc_result = res[1]

    party_1_proc.join()
    party_2_proc.join()

    # with open('party_0.csv','r') as f:
    #     f_csv=csv.reader(f)
    #     headers=next(f_csv)
    #     i=0
    #     for row in f_csv:
    #         print(row)
    #         print(type(row))
    #         print(row[0])

    #         col_A[i]=float(row[0])
    #         print(col_A[i])
    #         i+=1
    # f.close()

    # with open('party_1.csv','r') as f:
    #     f_csv=csv.reader(f)
    #     headers=next(f_csv)
    #     i=0
    #     for row in f_csv:
    #         col_B[i]=float(row[0])
    #         i+=1
    # f.close()

    # with open('party_2.csv','r') as f:
    #     f_csv=csv.reader(f)
    #     headers=next(f_csv)
    #     i=0
    #     for row in f_csv:
    #         col_C[i]=float(row[0])
    #         col_D[i]=float(row[1])
    #         i+=1
    # f.close()

    # Run local evaluate.
    local_exec = pybind_mpc.LocalExpressExecutor(mpc_exec)
    local_exec.import_column_values("A", col_A)
    local_exec.import_column_values("B", col_B)
    local_exec.import_column_values("C", col_C)
    local_exec.import_column_values("D", col_D)
    local_exec.finish_import()
    local_result = local_exec.evaluate()

    # compute the D-Value between mpc and plain-text
    dvalue = []
    for i in range(len(mpc_result)):
        dvalue.append(mpc_result[i] - local_result[i])

    # write csv file
    headers = ['A', 'B', 'C', 'local_result', 'mpc_result', 'dvalue']
    with open('result.csv', 'w') as f:
        f_csv = csv.writer(f)
        f_csv.writerow(headers)
        for i in range(len(mpc_result)):
            f_csv.writerow([col_A[i], col_B[i], col_C[i],
                            local_result[i], mpc_result[i], dvalue[i]])
    f.close()


# def mpc_run_with_grpc():
#     generator = express.MPCExpressRequestGenerator()
# 
#     jobid = "202002280915402308774"
#     filename = "/tmp/mpc_{}_result.csv".format(jobid)
# 
#     # Generate request for party 0.
#     generator.set_party_id(0)
#     generator.set_job_id(jobid)
#     generator.set_mpc_addr("192.168.99.21", "192.168.99.21", 10020, 10030)
#     generator.set_input_output("/home/primihub/expr/party_0.csv", filename)
#     generator.set_expr("A+B*C+D")
# 
#     generator.set_column_attr("A", 0, True)
#     generator.set_column_attr("B", 1, True)
#     generator.set_column_attr("C", 2, True)
#     generator.set_column_attr("D", 2, True)
# 
#     party_0_request = generator.gen_request()
# 
#     # Generate request for party 1.
#     generator.set_party_id(1)
#     generator.set_mpc_addr("192.168.99.22", "192.168.99.21", 10040, 10020)
#     generator.set_input_output("/home/primihub/expr/party_1.csv", filename)
# 
#     party_1_request = generator.gen_request()
# 
#     # Generate request for party 2.
#     generator.set_party_id(2)
#     generator.    ("192.168.99.21", "192.168.99.22", 10030, 10040)
#     generator.set_input_output("/home/primihub/expr/party_1.csv", filename)
# 
#     party_2_request = generator.gen_reqeust()
# 
#     # Send request to service.
#     party_0_response = MPCExpressServiceClient.start_task(
#         "192.168.99.21:50051", party_0_request)
#     party_1_response = MPCExpressServiceClient.start_task(
#         "192.168.99.22:50051", party_1_request)
#     party_2_response = MPCExpressServiceClient.start_task(
#         "192.168.99.28:50051", party_2_request)
# 
#     print(party_0_response)
#     print(party_1_response)
#     print(party_2_response)


if __name__ == '__main__':
    mpc_run_without_grpc()
