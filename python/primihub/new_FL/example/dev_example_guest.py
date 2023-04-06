import pandas as pd
class Dev_example_guest:
    @staticmethod
    def run(context):
        df = pd.DataFrame([1,2,3])
        df.to_csv("/home/primihub/czl/res.csv")
    

    def train(self, X, y):
        pass
    
    def predict(self, X):
        pass
