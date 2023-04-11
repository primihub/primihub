import pandas as pd
class Dev_example_host:
    @staticmethod
    def run(task_parameter, party_access_info):
        X = eval(task_parameter['data']['X'])
        print(pd.read_csv(X['data_path']))
    

    def train(self, X, y):
        pass
    
    def predict(self, X):
        pass
