import os
import csv
import pybind_mpc
import traceback
import multiprocessing
import random


def run_mpc_party(party_id, expr, col_owner, col_dtype,
                  col_val, party_addr, reveal_party):
    log_prefix=os.environ.get('LOG_PREFIX')
    log_dir=os.environ.get('LOG_DIR')
    if log_prefix==None:
        log_prefix="No Config"
    if log_dir==None:
        log_dir="No Config"
    else:
        if os.path.exists(log_dir)== False:
            os.mkdir(log_dir)
    mpc_exec = pybind_mpc.MPCExpressExecutor(party_id,log_prefix,log_dir)
        
    mpc_exec.import_column_config(col_owner, col_dtype)
    mpc_exec.import_express(expr)

    for name, val in col_val.items():
        mpc_exec.import_column_values(name, val)

    mpc_exec.evaluate(party_addr[0], party_addr[1],
                      party_addr[2], party_addr[3])
    result = mpc_exec.reveal_mpc_result(reveal_party)

    return result


def mpc_run_without_grpc():
    env_dict={}
    party_cols={}

    try:
        party_id=int(os.environ.get('PARTY_ID'))
        env_dict['party_id']=party_id

        expr=os.environ.get('EXPR')
        env_dict['expr']=expr

        owner=os.environ.get('COL_OWNER')
        col_owner=dict.copy(eval(owner))
        env_dict['col_owner']=owner

        dtype=os.environ.get('COL_DTYPE')
        col_dtype=dict.copy(eval(dtype))
        env_dict['col_dtype']=col_dtype

        partyaddr=os.environ.get('PARTY_ADDR')
        party_addr=(partyaddr.split(",")[0],partyaddr.split(",")[1],int(partyaddr.split(",")[2]),int(partyaddr.split(",")[3]))
        env_dict['party_addr']=party_addr

        revealparty = os.environ.get('REVEAL_PARTY')
        reveal_party=[int(i) for i in revealparty.split(",")]
        env_dict['reveal_party']=reveal_party

        data_name=os.environ.get('DATA_NAME')
        env_dict['data_name']=data_name

        res_file=os.environ.get('RES_FILE')
        env_dict['res_file']=res_file

        
        # data_name_A=os.environ.get('DATA_NAME_A')
        # env_dict['data_name_A']=data_name_A
        
        # data_name_B=os.environ.get('DATA_NAME_B')
        # env_dict['data_name_B']=data_name_B

        # data_name_C=os.environ.get('DATA_NAME_C')
        # env_dict['data_name_C']=data_name_C

        # res_file=os.environ.get('RES_FILE')
        # env_dict['res_file']=res_file
        
        with open(data_name,'r') as csvfile:
            f_csv = csv.reader(csvfile)
            headers=next(f_csv)
            for i in range(len(headers)):
                party_cols[headers[i]]=[]
            for row in f_csv:
                for i in range(len(headers)):
                    if col_dtype[headers[i]] == 0:
                        party_cols[headers[i]].append(int(row[i]))
                    else:
                        party_cols[headers[i]].append(float(row[i]))
        csvfile.close()

        mpc_result = run_mpc_party(party_id, expr, col_owner, col_dtype,
                  party_cols, party_addr, reveal_party)
        
        headers = ['mpc_result']
        res_file=str(party_id)+'_'+res_file
        with open(res_file, 'w') as f:
            f_csv = csv.writer(f)
            f_csv.writerow(headers)
            for i in range(len(mpc_result)):
                f_csv.writerow([mpc_result[i]])
        f.close()
    except Exception as e:
        print(e.args)
        print('=====')
        print(traceback.format_exc())


if __name__ == '__main__':
    mpc_run_without_grpc()