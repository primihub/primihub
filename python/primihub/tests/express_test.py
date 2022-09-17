import pympc
import random
import multiprocessing
import csv


def run_mpc_party(party_id, expr, col_owner, col_dtype,
                  col_val, party_addr, reveal_party):
    mpc_exec = pympc.MPCExpressExecutor(party_id)
    mpc_exec.import_column_config(col_owner, col_dtype)
    mpc_exec.import_express(expr)

    for name, val in col_val.items():
        mpc_exec.import_column_values(name, val)

    mpc_exec.evaluate(party_id, party_addr[0], party_addr[1], party_addr[2])
    result = mpc_exec.reveal_mpc_result(reveal_party)

    return mpc_exec, result


if __name__ == '__main__':
    reveal_party = [0, 1, 2]
    expr = "A+B*C+D"

    col_owner = {"A": 0, "B": 1, "C": 2, "D": 2}
    col_dtype = {"A": 1, "B": 1, "C": 1, "D": 1}

    col_A = [random.uniform(1, 10000) for i in range(1000)]
    col_B = [random.uniform(1, 10000) for i in range(1000)]
    col_C = [random.uniform(1, 10000) for i in range(1000)]
    col_D = [random.uniform(1, 10000) for i in range(1000)]

    party_0_cols = {"A": col_A}
    party_1_cols = {"B": col_B}
    party_2_cols = {"C": col_C, "D": col_D}

    # Start party 1.
    party_1_addr = ("127.0.0.1", 10030, 10010)
    party_1_args = (1, expr, col_owner, col_dtype,
                    party_1_cols, party_1_addr, reveal_party)

    party_1_proc = multiprocessing.Process(
        target=run_mpc_party, args=party_1_args)

    party_1_proc.daemon = True
    party_1_proc.start()

    # Start party 2.
    party_2_addr = ("127.0.0.1", 10020, 10030)
    party_2_args = (2, expr, col_owner, col_dtype,
                    party_2_cols, party_2_addr, reveal_party)

    party_2_proc = multiprocessing.Process(
        target=run_mpc_party, args=party_2_args)

    party_2_proc.daemon = True
    party_2_proc.start()

    # Start party 0.
    party_0_addr = ("127.0.0.1", 10010, 10020)
    res = run_mpc_party(0, expr, col_owner, col_dtype,
                        party_0_cols, party_0_addr, reveal_party)

    mpc_exec = res[0]
    mpc_result = res[1]

    # Run local evaluate.
    local_exec = pympc.LocalExpressExecutor(mpc_exec)
    local_exec.import_column_values("A", col_A)
    local_exec.import_column_values("B", col_B)
    local_exec.import_column_values("C", col_C)
    local_exec.import_column_values("D", col_D)
    local_exec.finish_import()
    local_result = local_exec.evaluate()
