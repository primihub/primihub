import pandas as pd
from primihub.new_FL.algorithm.net_work import GrpcServer
class Model:
    def __init__(self, task_parameter, party_access_info):
        self.task_parameter = task_parameter
        self.party_access_info = party_access_info
    
    def run(self):
        task_parameter = self.task_parameter
        party_access_info = self.party_access_info
        print(task_parameter)
        print(party_access_info)

        server = GrpcServer('Alice','Bob', party_access_info, task_parameter['task_info'])
        res = server.recv('abc1')
        print(res)
    

    def train(self, X, y):
        pass
    
    def predict(self, X):
        pass
