import pandas as pd
from primihub.new_FL.utils.net_work import GrpcServer
class Dev_example_guest:
    @staticmethod
    def run(task_parameter, party_access_info):
        print(task_parameter)
        print(party_access_info)

        server = GrpcServer('Alice','Bob', party_access_info, task_parameter['task_info'])
        res = server.recv('abc1')
        print(res)
    

    def train(self, X, y):
        pass
    
    def predict(self, X):
        pass
