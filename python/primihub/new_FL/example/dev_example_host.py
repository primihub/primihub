import pandas as pd
from primihub.new_FL.utils.net_work import GrpcServer

class Dev_example_host:
    @staticmethod
    def run(task_parameter, party_access_info):
        X = eval(task_parameter['data']['X'])
        print(pd.read_csv(X['data_path']))
        if task_parameter['party_name'] == 'Bob':
            server = GrpcServer('Alice', 'Bob', party_access_info, task_parameter['task_info'])
            server.sender('abc', 'efg')    

    def train(self, X, y):
        pass
    
    def predict(self, X):
        pass
