import express_test
import os
import csv
if __name__ == '__main__':
    env_dict={}
    party_cols={}
    try:
        party_id=int(os.environ.get('party_id'))
        env_dict['party_id']=party_id

        expr=os.environ.get('expr')
        env_dict['expr']=expr

        owner=os.environ.get('col_owner')
        col_owner=dict.copy(eval(owner))
        env_dict['col_owner']=owner

        dtype=os.environ.get('col_dtype')
        col_dtype=dict.copy(eval(dtype))
        env_dict['col_dtype']=col_dtype

        partyaddr=os.environ.get('party_addr')
        party_addr=(partyaddr.split(",")[0],partyaddr.split(",")[1],int(partyaddr.split(",")[2]),int(partyaddr.split(",")[3]))
        env_dict['party_addr']=party_addr

        revealparty = os.environ.get('reveal_party')
        reveal_party=[int(i) for i in revealparty.split(",")]
        env_dict['reveal_party']=reveal_party

        data_name=os.environ.get('data_name')
        env_dict['data_name']=data_name

        with open(data_name,'r') as csvfile:
            f_csv = csv.reader(csvfile)
            headers=next(f_csv)
            for i in range(len(headers)):
                party_cols[headers[i]]=[]
            for row in f_csv:
                for i in range(len(headers)):
                    party_cols[headers[i]].append(row[i])
        csvfile.close()
        print(party_cols)       
        for key in env_dict:
            if env_dict[key]==None:
                print("error!Please set environment variable: "+ key)
                exit(-1) 
        express_test.run_mpc_party(party_id, expr, col_owner, col_dtype,
                  party_cols, party_addr, reveal_party)
    except Exception as e:
        print(e)
