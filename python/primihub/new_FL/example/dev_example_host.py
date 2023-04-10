import pandas as pd
class Dev_example_host:
    @staticmethod
    def run(task_parameter, party_access_info):
        print(task_parameter)
        print(pd.read_csv(task_parameter['data']['X']['data_path']))
    

    def train(self, X, y):
        pass
    
    def predict(self, X):
        pass
