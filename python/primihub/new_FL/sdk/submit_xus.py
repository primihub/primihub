# from client import Client
from client_xus import Client
from copy import deepcopy
import sys
import json

# def generate_para_map(task_parameter, data, roles):
#     party2role = dict()
#     tmp_party_list = []
#     for role, party_list in roles.items():
#         for party in party_list:
#             party2role[party] = role
#             tmp_party_list.append(party)

#     #set the paramap
#     para_map = {}
#     for party in tmp_party_list:
#         tmp_param = deepcopy(task_parameter)
#         tmp_param['role'] = party2role[party]
#         tmp_param['data'] = data[party] if party in data else None
#         para_map[party] = tmp_param

#     return para_map

json_file = sys.argv[1]
if __name__ == '__main__':
    print(f"json_file is {json_file}")
    with open(json_file, 'r') as f:
        json_file = json.load(f)

    client = Client(json_file=json_file)
    client.submit()
    # party_access_info = json_file["party_access_info"]
    # roles = json_file["roles"]

    # client = Client(party_access_info['task_manager'], None, party_access_info)
    # for task in json_file['tasks']:

    #     #set commom paramerter
    #     data = task["data_path"]
    #     process = task['process']
    #     task_parameter = task["parameters"]
    #     task_parameter['model'] = task["model"]
    #     task_parameter['process'] = process
    #     task_parameter['protocol'] = 'FL'
    #     task_parameter['all_roles'] = roles
    #     #generate the task map
    #     para_map = generate_para_map(task_parameter, data, roles)

    #     #submit the task and get the task ID
    #     task_ids = client.submit(para_map)
