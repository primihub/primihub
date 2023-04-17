import pandas as pd
from primihub.new_FL.algorithm.net_work import GrpcServer
from primihub.new_FL.algorithm.FL_helper import FL_helper
class Model:
    def __init__(self, task_parameter, party_access_info):
        self.task_parameter = task_parameter
        self.party_access_info = party_access_info
        

    def run(self):
        task_parameter = self.task_parameter
        party_access_info = self.party_access_info
        X = eval(task_parameter['data']['X'])
        print(pd.read_csv(X['data_path']))
        if task_parameter['party_name'] == 'Bob':
            server = GrpcServer('Bob','Alice', party_access_info, task_parameter['task_info'])
            server.sender('abc1', 'efg')    

        helper = FL_helper(task_parameter,party_access_info)
        print(helper.get_roles())
        print(helper.get_parties())
        print(helper.role2party('guest'))
        print(helper.party2role('Alice'))


    def train(self, X, y):
        pass
    
    def predict(self, X):
        pass
